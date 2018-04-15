
= GoではじめるElasticsearch

== はじめに


Elasticsearchの入門の多くはREST APIを使ったものが多いのですが、実際にアプリケーションを作成する際は何らかの言語のSDKを利用するかと思います。
そうした際に意外と「あれ、これってどうやるんだ？」となる場合が多いものです。

そこで、本章ではElasticsearchの基本操作を、Go言語を利用して体験していきます。Elasticsearchの基本的な操作を中心に、ちょっとしたTipsについても触れていきます。
Elasticsearchはとても多くの機能を有しています。そのため、全ての機能をカバーすることは難しいです。よって、代表的な機能について本章では記載します。 また本章ではElasticsearchのAPIを主に扱います。


== Elasticsearch環境の準備


今回はElastic社が提供している公式Dockerイメージを利用します。
下記のコマンドを実行してDockerイメージを取得してください。


//list[elasticsearch-list01][Dockerイメージの取得]{
docker pull docker.elastic.co/elasticsearch/elasticsearch:6.2.2
//}


Dockerイメージが起動できるかを確認します。この章の後半でElasticsearchの外部プラグインをインストールします。
Dockerイメージは停止するとイメージ内のデータは消えてしまいます。そのため本書ではインストールしたプラグインを保存する先としてpluginsディレクトリを作成し、
Dockerイメージの起動時にマウントさせて利用します。ローカルPC上に作成したpluginsディレクトリが存在する場所でDockerイメージの起動をおこなってください。


//list[elasticsearch-list02][Dockerイメージの起動]{
docker run -p 9200:9200  -e "discovery.type=single-node" -e （紙面の都合で改行）
"network.publish_host=localhost"（紙面の都合で改行）
-v plugins:/usr/share/elasticsearch/plugins（紙面の都合で改行）
（紙面の都合により改行）docker.elastic.co/elasticsearch/elasticsearch:6.2.2
//}


起動に成功すると、プロンプト上に起動ログが出力されます。
ポートマッピングで指定している9200ポートはElasticsearchへのAPIを実行するためのエンドポイントです。
Elastic社のDockerイメージを利用すると、Docker起動時に環境変数経由でElasticsearchの設定を変更できます。

起動時にいくつかオプションを指定しているため解説します。
まず、オプション@<code>{discovery.type}を@<code>{single-node}に設定しています。
このElasticsearchはクラスタを構成せず、シングルノード構成であることを明示します。すると、起動時に自分自身をマスタノードとして設定し起動します。

次に、@<code>{network.publish_host}を@<code>{loccalhost}に設定しました。
ElasticsearchのAPIエンドポイントとして公開するIPアドレスを指定します。
指定しなかった場合、Dockerコンテナ内部のプライベートIPアドレスとなり、
ローカルホストから直接Elasticsearchのエンドポイントへ接続することができないため、この設定を入れています。


Dockerが正常に起動しているか確認してみましょう。さきほどマッピングした9200ポートでElasticsearchはREST APIのエンドポイントを公開しています。
@<list>{elasticsearch-list03}を用いてElasticsearchの基本情報について取得できるか確認してください。


//list[elasticsearch-list03][Elasticsearchの起動確認]{
curl http://localhost:9200
//}

//cmd{

# curl http://localhost:9200
{
  "name" : "7JNxM8W",
  "cluster_name" : "docker-cluster",
  "cluster_uuid" : "uaHKm_QGR6yzRCbH87JIcA",
  "version" : {
    "number" : "6.2.2",
    "build_hash" : "10b1edd",
    "build_date" : "2018-02-16T19:01:30.685723Z",
    "build_snapshot" : false,
    "lucene_version" : "7.2.1",
    "minimum_wire_compatibility_version" : "5.6.0",
    "minimum_index_compatibility_version" : "5.0.0"
  },
  "tagline" : "You Know, for Search"
}

//}


ElasticsearchのDockerイメージの起動オプションなどはDockerHubのドキュメント（@<href>{https://hub.docker.com/_/elasticsearch/}）に記載があります。


== クライアントライブラリの選定


まずはElasticsearchを操作するためのクライアントライブラリを決める必要があります。
Elastic社の公式クライアント@<href>{https://github.com/elastic/go-elasticsearch}もあるのですが、現時点では絶賛開発中なうえにあまり活発にメンテナンスもされていません…。


今回は@<code>{Elastic:An Elasticsearch client for the Go}（@<href>{https://github.com/olivere/elastic}）を利用します。
こちらのクライアントは開発も活発で、Elasticの早いバージョンアップにもいち早く対応されています。

本書で扱う内容もolivere/elasticのGetting Started(@<href>{https://olivere.github.io/elastic/})をもとにしているため、より多くの機能の使い方などを知るためにもぜひこちらもご参照ください。

それではクライアントをインストールしましょう。
今回はgo getでインストールしますが、実際のプロダクト利用時はdepなどのパッケージ管理ツールの利用をおすすめします。
Goのインストール及びGOPATHの設定を事前にお願いします。


//list[elasticsearch-list04][Elasticクライアントのインストール]{
go get "github.com/olivere/elastic"
//}


== Elasticsearchでの準備


さて、いよいよGoでElasticsearchを操作していきましょう。
しかしその前に検索するデータを投入するためのIndexとTypeを作成していきます。


=== IndexとType


Elasticsearchで検索をおこなうために、まずIndexとTypeを作成する必要があります。
RDBMSで例えると以下に相当します。

 * Indexはスキーマ/データベース
 * Typeはテーブル


と、このようにRDBMSで例えられることが多いのですが、TypeはElasticsearch 7系より廃止が予定されています。
また5系までは1つのIndexに複数のTypeを作成できたのですが、6系では1つのIndexに1つのTypeのみ作成できる仕様へ変わっています
（参考：@<href>{https://www.elastic.co/guide/en/elasticsearch/reference/master/removal-of-types.html}）。

本章ではElasticsearch6系を利用するため、1 Indexに1 Typeを作成します。
また、ElasticsearchはMapping定義を作成しなくてもデータを投入することもできます。

その際は投入したJSONデータにあわせたMappingが自動で作成されます。
実際の検索アプリケーションでElasticsearchを利用する場合、Mapping定義によりデータスキーマを固定して利用することの方が多いかと思います。
また、Mapping定義を作成することにより、各フィールド単位でより細かな検索設定をおこなうことが可能なため本書ではMapping定義を1から作成し利用します。


=== Mapping


本章ではChatアプリケーションを想定したIndex/Typeをもとに操作をおこなっていきます。

Elasticsearchの操作に必要なMapping定義を@<list>{elasticsearch-list05}に記述しました。


//list[elasticsearch-list05][利用するMapping定義]{
{
  "mappings": {
    "chat": {
      "properties": {
        "user": {
          "type": "keyword"
        },
        "message": {
          "type": "text"
        },
        "created": {
          "type": "date"
        },
        "tag": {
          "type": "keyword"
        }
      }
    }
  }
}
//}


今回は@<code>{chat}というTypeへドキュメントを登録していきます。また、@<code>{properties}にフィールドの項目を設定します。
フィールド名とそのデータ型を@<code>{type}で指定していきます。今回指定しているデータ型について説明します。

@<code>{keyword}はいわゆるString型です。後述するtext型もString型に相当します。しかしkeyword型の場合、そのフィールドへアナライザは適用されません。

@<code>{text}はString型に相当します。text型を指定したフィールドはアナライザと呼ばれるElasticsearchの高度な検索機能を利用した検索が可能となります。

@<code>{date}は日付型です。Elasticsearchへのデータ投入はJSONを介して行うため、実際にデータを投入する際はdateフォーマットに即した文字列を投入することになります。


keyword型とtext型は両者ともString型に相当します。その違いはアナライザを設定できるか否かです。
後ほど詳細を説明しますが、アナライザを適用することでそのフィールドに対し高度検索を行うことができます。一方でkeyword型はアナライザが適用されないため、完全一致での検索が求められます。
また、フィールドに対してソートをおこなう場合、keyword型を指定する必要があります。


Elasticsearch 6系のデータ型の詳細は公式ドキュメント（@<href>{https://www.elastic.co/guide/en/elasticsearch/reference/current/mapping-types.html}）を参照してください。
多くのデータ型が標準でサポートされています。


それでは、このMapping定義をElasticsearchへ投入します。先ほどのMapping定義を@<code>{mapping.json}として保存してください。
本書ではcurlコマンドを利用しElasticsearchのAPIを実行します。


//list[elasticsearch-list0][Mappingの作成]{
curl -XPUT 'http://localhost:9200/<Index名>' -H "Content-Type: application/json" -d @mapping.json
//}


Index名に作成するIndexの名前を指定し、先ほど作成したMapping定義をPUTします。本書ではIndexとTypeの両方をchatとします。


//cmd{
# curl -XPUT http://localhost:9200/chat -H "Content-Type: application/json" -d @mapping.json
{"acknowledged":true,"shards_acknowledged":true,"index":"chat"}⏎
//}


作成されたIndexを確認します。下記のエンドポイントから指定したIndex/TypeのMapping定義を確認できます。


//list[elasticsearch-list00][Mappingの確認]{
curl -XGET 'http://localhost:9200/<Index名>/_mapping/<Type名>?pretty'
//}


//cmd{
# curl -XGET 'http://localhost:9200/chat/_mapping/chat?pretty'
{
  "chat" : {
    "mappings" : {
      "chat" : {
        "properties" : {
          "created" : {
            "type" : "date"
          },
          "message" : {
            "type" : "text"
          },
          "tag" : {
            "type" : "keyword"
          },
          "user" : {
            "type" : "keyword"
          }
        }
      }
    }
  }
}
//}


== Hello, Elasticsearch with Go


=== Elasticsearchにつないでみよう


それではGoを使ってElasticsearchを操作していきましょう。
まず始めに、さきほどDockerで起動したElasticsearchへの接続確認をおこなうため、Elasticsearchのバージョン情報などを取得します。


//list[elasticsearch-list07][Go言語を用いてElasticsearchに接続する(hello_elasticsearch.go)]{
package main

import (
        "context"
        "fmt"

        "github.com/olivere/elastic"
)

func main() {
        esURL := "http://localhost:9200"
        ctx := context.Background()

        client, err := elastic.NewClient(
                elastic.SetURL(esURL),
        )
        if err != nil {
                panic(err)
        }

        info, code, err := client.Ping(esURL).Do(ctx)
        fmt.Printf("Elasticsearch version %s\n", info.Version.Number)
}
//}


@<code>{elastic.NewClient}でクライアントを作成します。その際に@<code>{elastic.ClientOptionFunc}で複数の設定を引数とすることが可能です。
@<list>{elasticsearch-list07}では@<code>{elastic.SetURL()}にて接続する先のElasticsearchのエンドポイントを指定しています。
クライアントを作成すると、そのオブジェクトを通じてElasticsearchを操作することができるようになります。
Elasticsearchのバージョン情報といったシステム情報を取得する際は@<code>{Ping}を利用します。


では実行してみましょう!


//cmd{
$ go run hello_elasticsearch.go
Elasticsearch version 6.2.2
//}


ローカル環境で稼働させているElasticsearchのバージョンが表示されれば、Elasticsearchに接続できています。
もし接続できない場合、正常にElasticsearchのコンテンが起動しているか、ポートマッピングが正しくおこなわれているかなどを確認してください。
また以下のように、クライアント作成時に以下のオプションを付与して試してください。(以降のサンプルでも同様)


//list[elasticsearch-list08][もしElasticsearchへの接続に失敗する場合]{
client, err := elastic.NewClient(
     elastic.SetURL(esURL),
	 //sniff機能を無効化
	 elastic.SetSniff(false),
)
//}


=== 単純なCRUD操作


それでは@<list>{elasticsearch-list0}で作成したIndexを対象に基本的なCRUD操作をおこなってみましょう。
操作を始めるために、まずはクライアントのオブジェクトを作成します。



このクライアントオブジェクトを通じてElasticsearchを操作していきます。
クライアントの作成時に以下の2つのオプションを指定しています。
特にSetSniffはElasticsearchのDockerコンテナへ接続する際に必要となる設定です。



操作にあたっては、さきほど作成したMappingに対応するStructを通じておこなっていきます。
よって、今回サンプルとして利用するChat Mappingに対応するStructを定義します。


//list[elasticsearch-list20][Structの定義]{
type Chat struct {
    User    string    `json:"user"`
    Message string    `json:"message"`
    Created time.Time `json:"created"`
    Tag     string    `json:"tag"`
}
//}


GoのクライアントとElasticsearch間はHTTP(S)で通信され、JSONでデータのやり取りがおこなわれます。
そのため、StructにはMappingで定義したフィールド名をjsonタグで指定することでMapping定義上のフィールド名とマッピングします。


==== ドキュメントの登録


まずは単一のドキュメントを登録します。
Elasticsearchは登録されたドキュメントに対して、ドキュメントを一意に識別するための@<code>{ドキュメントID}を付与します。
IDの振り方には登録時にクライアント側で設定するか、Elasticsearch側でランダムに振ってもらうかの2通りがあります。
今回は登録時にクライアント側でドキュメントIDを指定します。
さきほど作成したクライアントセッションを利用して操作をおこなっていきましょう。

//list[elasticesearch-list08][ドキュメントの登録(index.go)]{
package main

import (
        "context"
        "fmt"
        "time"

        "github.com/olivere/elastic"
)

type Chat struct {
        User    string    `json:"user"`
        Message string    `json:"message"`
        Created time.Time `json:"created"`
        Tag     string    `json:"tag"`
}

func main() {
        esURL := "http://localhost:9200"
        ctx := context.Background()
        client, err := elastic.NewClient(
                elastic.SetURL(esURL),
        )
        if err != nil {
                panic(err)
        }

        //登録するドキュメントを作成
        chatData := Chat{
                User:    "user01",
                Message: "test message",
                Created: time.Now(),
                Tag:     "tag01",
        }

        //ドキュメントIDを1として登録
        indexedDoc, err := client.Index().Index("chat").（紙面の都合により改行）
				Type("chat").Id("1").BodyJson(&chatData).Do(ctx)
        if err != nil {
                panic(err)
        }
        fmt.Printf("Index/Type: %s/%sへドキュメント(ID: %s)が登録されました\n", indexedDoc.Index, indexedDoc.Type, indexedDoc.Id)
}
//}


//cmd{
$ go run index.go
Index/Type: chat/chatへドキュメント(ID: 1)が登録されました
//}


==== ドキュメントIDによる取得


次に先ほど登録したドキュメントを、@<code>{ドキュメントID}を指定して取得します。
@<code>{Elastic:An Elasticsearch client for the Go}では取得したドキュメントはStrucrtに保存し直し、そのStructのフィールドを経由してデータを取得できます。


//list[elasticsearch-list09][ドキュメントの取得(get.go)]{

package main

import (
        "context"
        "encoding/json"
        "fmt"
        "time"

        "github.com/olivere/elastic"
)

type Chat struct {
        User    string    `json:"user"`
        Message string    `json:"message"`
        Created time.Time `json:"created"`
        Tag     string    `json:"tag"`
}

func main() {
        esURL := "http://localhost:9200"
        ctx := context.Background()

        client, err := elastic.NewClient(
                elastic.SetURL(esURL),
        )
        if err != nil {
                panic(err)
        }

        document, err := client.Get().Index("chat").Type("chat").Id("1").Do(ctx)
        if err != nil {
                panic(err)
        }

        if document.Found {
                var chat Chat
                err := json.Unmarshal(*document.Source, &chat)
                if err != nil {
                        fmt.Println(err)
                }

                fmt.Printf("Message:<%s> created by %s \n", chat.Message, chat.User)
        }
}
//}


//cmd{
$ go run get.go
essage:<test message> created by user01
//}


==== ドキュメントの削除


ドキュメントIDをもとに登録したドキュメントを削除します。
登録したドキュメントを、@<code>{ドキュメントID}を指定して取得します。


//list[elasticsearch-list10][ドキュメントの削除(delete.go)]{
package main

import (
        "context"
        "fmt"
        "time"

        "github.com/olivere/elastic"
)

type Chat struct {
        User    string    `json:"user"`
        Message string    `json:"message"`
        Created time.Time `json:"created"`
        Tag     string    `json:"tag"`
}

func main() {
        esURL := "http://localhost:9200"
        ctx := context.Background()

        client, err := elastic.NewClient(
                elastic.SetURL(esURL),
        )
        if err != nil {
                panic(err)
        }

        deletedDoc, err := client.Delete().Index("chat").Type("chat").Id("1").Do(ctx)
        if err != nil {
                panic(err)
        }

        fmt.Println(deletedDoc.Result)
}
//}


//cmd{
$ go run delete.go
deleted
//}


== 検索の基本


さて、基本的なCRUDを通じてElasticsearchの基本をおさえたところで、検索処理について詳しく掘り下げていきます。
Elasticsearchは多くの検索機能をサポートしています。本章ではその中でも代表的な昨日について取り上げます。
Elasticsearchの高度な検索を支える仕組みにAnalyzerがあります。これらの検索クエリもAnalyzerの機能を利用することで、より柔軟な検索をおこなうことができます。
まずはElasticsearchのAnalyzerについてみていきましょう。

 * Match Query
 ** 指定した文字列での全文検索をおこないます。検索時に指定した文字列はAnalyzerにより言語処理がなされたうえで、検索がおこなれます。
 * Term Query
 ** 指定した文字列での検索をおこないますが、Match Queryとは違い検索指定文字列がAnalyzeされません。例えば、タグ検索のように指定した文字列で完全一致させたドキュメントを探したい時などはTerm Queryを利用するといったケースです。
 * Bool Query
 ** AND/OR/NOTによる検索がおこなえます。実際にはmust/should/must_notといったElasticsearch独自の指定方法を利用します。検索条件をネストさせることも可能で、より複雑な検索Queryを組み立てることができます。


=== Analyzerの基本


ここでAnalyzerについて簡単に説明します。Analyzerの設定は全文検索処理の要です。そのため、設定内容も盛り沢山ですし、自然言語処理の知識も必要となってくるため、ここではあくまで触りだけを説明します。
この本をきっかけにElasticsearchにもっと興味を持っていただけた方はAnalyzerを深掘ってみてください。


Analyzerは以下の要素から構成されています。これらを組み合わせることでより柔軟な検索をおこなうためのインデックスを作成することが可能です。

 * Tokenizer
 ** ドキュメントをどのようにトークン分割するかを定義します。トークン分割には様々な方法があり、有名なものだと形態素解析やN-Gramなどがあります。Tokenizerにより分割されたトークンをもとに検索文字列との比較がおこなわれます。各Analyzerは1つのTokenizerを持つことができます。
 * Character filters
 ** Tokenizerによるトークン分割がされる前に施す処理を定義します。例えば検索文字列のゆらぎを吸収するために、アルファベットの大文字・小文字を全て小文字に変換したり、カタカナの全角・半角を全て半角に統一したりといった処理をトークン分割の前処理として実施します。
 * Token filters
 ** Tokenizerによるトークン分割がされた後に施す処理を定義します。例えば、形態素解析のように品詞をもとにトークン分割する場合、分割後のトークンから検索には不要と思われる助詞を取り除くといった処理が該当します。


//image[analyzer_basic][Analyzerの構成要素]{
//}


Tokenizerで形態素解析を用いた場合の例を@<img>{analyzer_sample}に示します。


//image[analyzer_sample][Analyzerの仕組み]{
//}


このようにTokenizerだけでなく任意のFiltersを組みわせることで、検索要件に適したAnalyzerを作成し適用することができます。
本書では日本語形態素解析プラグインであるKuromojiを利用しAnalyzerの設定をおこなっていきます。


==== Kuromojiプラグインの導入


Kuromojiプラグインは標準ではElasticsearchに内蔵されていないため、追加でプラグインをインストールする必要があります。
稼働しているDockerのコンテナのコンテナIDを調べbashからプラグインのインストールをおこなっていきましょう。
Elasticsearchではプラグインをインストールする際には@<code>{elasticsearch-plugin}を利用します。
またプラグインを有効にするためにプラグインインストール後にコンテナの再起動を実施してください。


//cmd{
# docker ps

CONTAINER ID        IMAGE
9a96bafde5bd        docker.elastic.co/elasticsearch/elasticsearch-oss:6.0.0

COMMAND                  CREATED             STATUS
"/usr/local/bin/dock…"   2 hours ago         Up 2 hours

PORTS                                            NAMES
0.0.0.0:9200->9200/tcp, 0.0.0.0:9300->9300/tcp   agitated_haibt

# docker exec -it 66cec7c14657 bash
[root@9a96bafde5bd elasticsearch]# bin/elasticsearch-plugin install analysis-kuromoji
//}


==== MappingへのAnalyzerの適用


先ほど作成したMapping定義をもとにAnalyzerの設定を加えていきましょう。
Analyzerの設定は@<code>{settings}内でおこなっていきます。Analyzerを適用したいフィールドに@<code>{analyzer}を指定することで適用できます。


//list[elasticsearch-list11][Analyzerの設定(analyzed_mapping.json)]{

{
  "settings": {
    "analysis": {
      "analyzer": {
        "kuromoji_analyzer": {
          "type": "custom",
          "tokenizer": "kuromoji_tokenizer",
          "char_filter": [
            "kuromoji_iteration_mark"
          ],
          "filter": [
            "kuromoji_baseform",
            "kuromoji_part_of_speech",
            "ja_stop",
            "kuromoji_number",
            "kuromoji_stemmer"
          ]
        }
      }
    }
  },
  "mappings": {
    "chat": {
      "properties": {
        "user": {
          "type": "keyword"
        },
        "message": {
          "type": "text",
          "analyzer": "kuromoji_analyzer"
        },
        "created": {
          "type": "date"
        },
        "tag": {
          "type": "keyword"
        }
      }
    }
  }
}

//}

Analyzerの設定はMapping定義のanalysisでおこないます。tokenizerでトークン分割の方法を設定し、analyzerで設定したtoknenizerと各filter群を組み合わせて一つのAnalyzerを作ります。
本書では以下の設定でAnalyzerを設定しました。



//table[analyzer][本書で利用するAnalyzer]{
分類	分類	説明
-------------------------------------------------------------
Character Filters	kuromoji_iteration_mark	踊り字を正規化 e.g) すゝめ→すすめ
Tokenizer	kuromoji_tokenizer	日本語での形態素解析により文章をトークン化
Token Filters	kuromoji_baseform	動詞など活用になりかわる言葉を原形に変更 e.g) 読め→読む
Token Filters	kuromoji_part_of_speech	助詞など検索時に利用されない品詞を削除
Token Filters	ja_stop	文章中に頻出するor検索で利用されることがない言葉を削除 e.g) あれ、それ
Token Filters	kuromoji_number	漢数字を数字に変更 e.g) 五->5
Token Filters	kuromoji_stemmer	単語の末尾につく長音を削除 e.g) サーバー→サーバ
//}


作成したAnalyzerを適用したいMappingフィールドに指定することで、そのフィールドにAnalyzerで指定したインデクシングを施すことができます。
Chatマッピングの１階層下に存在する、messageフィールドの@<code>{analyzer}にさきほど作成した@<code>{Analyzer}を指定することで適用します。



ここではMapping定義を再作成します。


//list[elasticsearch-list12][Mapping定義の再作成]{
# curl -XDELETE 'http://localhost:9200/chat'
# curl -XPUT 'http://localhost:9200/chat' （紙面の都合により改行）
-H "Content-Type: application/json" -d @analyzed_mapping.json
//}


作成しなおしたインデックスに確認用のデータを登録します。(登録するデータがいささいか少ないですが、、すいません)


//list[elasticesearch-list112][テストデータの登録]{

package main

import (
	"context"
	"time"

	"github.com/olivere/elastic"
)

type Chat struct {
	User    string    `json:"user"`
	Message string    `json:"message"`
	Created time.Time `json:"created"`
	Tag     string    `json:"tag"`
}

func main() {
	esURL := "http://localhost:9200"
	ctx := context.Background()
	client, err := elastic.NewClient(
		elastic.SetURL(esURL),
	)
	if err != nil {
		panic(err)
	}
	chatData01 := Chat{
		User:    "user01",
		Message: "明日は期末テストがあるけどなんにも勉強してない....",
		Created: time.Now(),
		Tag:     "試験",
	}
	chatData02 := Chat{
		User:    "user02",
		Message: "時々だけど勉強のやる気が出るけど長続きしない",
		Created: time.Now(),
		Tag:     "学習",
	}
	chatData03 := Chat{
		User:    "user03",
		Message: "あと十年あれば期末テストもきっと満点がとれたんだろうな",
		Created: time.Now(),
		Tag:     "試験",
	}
	chatData04 := Chat{
		User:    "user04",
		Message: "ドラえもんの映画で一番すきなのは夢幻三剣士だな",
		Created: time.Now(),
		Tag:     "ドラえもん",
	}
	chatData05 := Chat{
		User:    "user05",
		Message: "世界記憶の概念、そうアカシックレコードを紐解くことで解は導かれるのかもしれない",
		Created: time.Now(),
		Tag:     "ファンタジー",
	}
	_, err = client.Index().Index("chat").Type("chat").Id("1").BodyJson(&chatData01).Do(ctx)
	_, err = client.Index().Index("chat").Type("chat").Id("2").BodyJson(&chatData02).Do(ctx)
	_, err = client.Index().Index("chat").Type("chat").Id("3").BodyJson(&chatData03).Do(ctx)
	_, err = client.Index().Index("chat").Type("chat").Id("4").BodyJson(&chatData04).Do(ctx)
	_, err = client.Index().Index("chat").Type("chat").Id("5").BodyJson(&chatData05).Do(ctx)
	if err != nil {
		panic(err)
	}
}

//}



これで準備が整いました！それではここの詳細に移っていきましょう。


=== Match Query


atchQueryは全文検索の肝です。MatchQueryでは、指定した検索文字列がAnalyzerにより言語処理がなされ検索がおこなわれます。
olivere/elasticで検索機能を利用する際は、client経由でSearchメソッドを実行します。
Searchメソッドはelastic.SearchServiceのQueryメソッドに、検索条件を指定した@<code>{elastic.MatchQuery}を代入します。
取得できたドキュメントをStruct経由で操作する際はreflectパッケージを使って操作します。


//list[elasticsearch-list13][Match Queryによる検索(match_query.go)]{
package main

import (
	"context"
	"fmt"
	"reflect"
	"time"

	"github.com/olivere/elastic"
)

type Chat struct {
	User    string    `json:"user"`
	Message string    `json:"message"`
	Created time.Time `json:"created"`
	Tag     string    `json:"tag"`
}

func main() {
	esURL := "http://localhost:9200"
	ctx := context.Background()
	client, err := elastic.NewClient(
		elastic.SetURL(esURL),
		elastic.SetSniff(false),
	)
	if err != nil {
		panic(err)
	}

	//messageフィールドに対して"テスト"という単語を含むドキュメントを検索
	query := elastic.NewMatchQuery("message", "テスト")
	results, err := client.Search().Index("chat").Query(query).Do(ctx)
	if err != nil {
		panic(err)
	}

	var chattype Chat
	for _, chat := range results.Each(reflect.TypeOf(chattype)) {
		if c, ok := chat.(Chat); ok {
			fmt.Printf("Chat message is: %s \n", c.Message)
		}
	}
}
//}


実行すると以下の２つのドキュメントがヒットします。


//cmd{
# go run match_query.go
Chat message is: あと十年あれば期末テストもきっと満点がとれたんだろうな
Chat message is: 明日は期末テストがあるけどなんにも勉強してない....
//}


意図した通りのドキュメントを取得することができました！では、この検索結果はどのように導かれたのでしょうか。
AnalyzerこれらのドキュメントがどのようにAnalyzeされインデクシングされているのか確認します。



//list[elasticsearch-list033][analyze api]{
# curl -XPOST "http://localhost:9200/<Index名>/_analyze?pretty" -H "Content-Type: application/json" -d
  '{
    "analyzer": "Analyzer名",
    "text": "Analyzeしたい文字列"
  }'
//}

//cmd{

# curl -XPOST "http://localhost:9200/chat/_analyze?pretty" -H "Content-Type: application/json" -d '{"analyzer": "kuromoji_analyzer", "text": "あと十年あれば期
末テストもきっと満点がとれたんだろうな"}'
{
  "tokens" : [
    {
      "token" : "あと",
      "start_offset" : 0,
      "end_offset" : 2,
      "type" : "word",
      "position" : 0
    },
    {
      "token" : "10",
      "start_offset" : 2,
      "end_offset" : 3,
      "type" : "word",
      "position" : 1
    },
    {
      "token" : "年",
      "start_offset" : 3,
      "end_offset" : 4,
      "type" : "word",
      "position" : 2
    },
    {
      "token" : "期末",
      "start_offset" : 7,
      "end_offset" : 9,
      "type" : "word",
      "position" : 5
    },
    {
      "token" : "テスト",
      "start_offset" : 9,
      "end_offset" : 12,
      "type" : "word",
      "position" : 6
    },
    {
      "token" : "きっと",
      "start_offset" : 13,
      "end_offset" : 16,
      "type" : "word",
      "position" : 8
    },
    {
      "token" : "満点",
      "start_offset" : 16,
      "end_offset" : 18,
      "type" : "word",
      "position" : 9
    },
    {
      "token" : "とれる",
      "start_offset" : 19,
      "end_offset" : 21,
      "type" : "word",
      "position" : 11
    }
  ]
}

//}


「あと十年あれば期末テストもきっと満点がとれたんだろうな」は設定したAnalyzerによりこのようにトークン化されインデクシングされています。
この中に「テスト」というトークンが含まれているために意図通りヒットしたというわけです。


=== Term Query


TermQueryを利用することで、指定した文字列を完全に含むドキュメントを検索することができます。
atchQueryと違い、検索文字列がAnalyzeされないため、指定した文字列と完全に一致する転地インデックスを検索します。
そのため、例えばタグ情報など指定した検索文字列と完全に一致させて検索をさせたい際に利用します。
Elastic:An Elasticsearch client for the GoでTermQueryを利用する際はTerm Queryは@<code>{elastic.TermQuery}を利用します。
elastic.NewTermQueryは検索対象のフィールドと検索文字列を指定します。


//list[elasticsearch-list14][Term Queryによる検索(term_query.go)]{
package main

import (
	"context"
	"fmt"
	"reflect"
	"time"

	"github.com/olivere/elastic"
)

type Chat struct {
	User    string    `json:"user"`
	Message string    `json:"message"`
	Created time.Time `json:"created"`
	Tag     string    `json:"tag"`
}

func main() {
	esURL := "http://localhost:9200"
	ctx := context.Background()
	client, err := elastic.NewClient(
		elastic.SetURL(esURL),
		elastic.SetSniff(false),
	)
	if err != nil {
		panic(err)
	}

	//タグに「ドラえもん」をもつドキュメントを取得
	termQuery := elastic.NewTermQuery("tag", "ドラえもん")
	results, err := client.Search().Index("chat").Type("chat").Query(termQuery).Do(ctx)
	if err != nil {
		panic(err)
	}

	var chattype Chat
	for _, chat := range results.Each(reflect.TypeOf(chattype)) {
		if c, ok := chat.(Chat); ok {
			fmt.Printf("Tag: %s and Chat message is: %s \n", c.Tag, c.Message)
		}
	}
}
//}


実行すると以下のドキュメントがヒットします。


//cmd{
# go run term_query.go
Tag: tag01 and Chat message is: ドラえもんの映画で一番すきなのは夢幻三剣士だな
//}


=== Bool Query


BoolQueryでは、MatchQueryやTermQueryなどを組み合わせたAND/OR/NOTによる検索をおこなうことが可能です。検索条件をネストさせることも可能で、より複雑な検索Queryを組み立てることができます。
実際にはmust/should/must_notといった、Elasticsearch独自の指定方法を利用します。


//table[tbl3][]{
Query	説明	oliver/elasticでの指定方法
-----------------
must	ANDに相当	boolQuery := elastic.NewBoolQuery()@<br>{}boolQuery.Must(elastic.NewTermQuery("field", "value")
should	ORに相当	boolQuery := elastic.NewBoolQuery()@<br>{}boolQuery.Should(elastic.NewTermQuery("field", "value")
must_not	NOT	boolQuery := elastic.NewBoolQuery()@<br>{}boolQuery.MustNot(elastic.NewTermQuery("field", "value")
//}


//list[elasticsearch-list15][Bool Queryによる検索(bool_query.go)]{

package main

import (
	"context"
	"fmt"
	"reflect"
	"time"

	"github.com/olivere/elastic"
)

type Chat struct {
	User    string    `json:"user"`
	Message string    `json:"message"`
	Created time.Time `json:"created"`
	Tag     string    `json:"tag"`
}

func main() {
	esURL := "http://localhost:9200"
	ctx := context.Background()
	client, err := elastic.NewClient(
		elastic.SetURL(esURL),
		elastic.SetSniff(false),
	)
	if err != nil {
		panic(err)
	}

	boolQuery := elastic.NewBoolQuery()
	//messageに「テスト」もしくは「勉強」を含み、user01のメッセージ以外を検索
	boolQuery.Should(
		elastic.NewMatchQuery("message", "テスト"),
		elastic.NewMatchQuery("message", "試験"),
	)
	boolQuery.MustNot(elastic.NewTermQuery("user", "user01"))
	results, err := client.Search().Index("chat").Query(boolQuery).Do(ctx)
	if err != nil {
		panic(err)
	}

	var chattype Chat
	for _, chat := range results.Each(reflect.TypeOf(chattype)) {
		if c, ok := chat.(Chat); ok {
			fmt.Printf("Chat message is: %s \n", c.Message)
		}
	}
}

//}


実行すると以下のドキュメントがヒットします。


//cmd{
# go run bool_query.go
Cnat message is: あと十年あれば期末テストもきっと満点がとれたんだろうな
//}


== ちょっと応用


ここでは少し応用的な機能についてみていきます。

 * Scroll API
 ** Elasticsearchが提供しているページング機能です。limit&offsetと違い、検索時のスナップショットを保持し、カーソルを利用してページの取得をおこないます。
 * Multi Fields
 ** Multi Fieldsタイプを指定することで1つのフィールドに対してデータ型やAnalyze設定が異なる複数のフィールドを保持することができます。
 * エラーハンドリング
 ** olivere/elasticを使った際のエラーハンドリングの方法について説明します。


利用するIndexは「検索の基本」で作成したものを引き続き利用します。


=== Scroll API


Scroll APIを利用することで、スクロールタイプのページング機能を手軽に利用することができます。Elasticsearchでは@<code>{limit&offset}を用いた値の取得もできます。
ただし、@<code>{limit&offset}を利用した場合、検索がおこなわれる度に指定したoffsetからlimit数分のドキュメントを取得します。そのため、取得結果に抜け漏れや重複が生じる可能性があります。
一方でScroll APIを利用した場合、初回検索時にスナップショットが生成されます。そのため、Scroll APIが返すスクロールIDを利用することで、初回検索時のスナップショットに対して任意の箇所からページングをおこなうことができます。
使い方はとても簡単で、@<code>{elastic.ScrollService}を介して操作することが可能です。


//list[elasticsearch-list16][Scroll APIを利用した検索(scroll_api.go)]{

package main

import (
	"context"
	"fmt"
	"reflect"
	"time"

	"github.com/olivere/elastic"
)

type Chat struct {
	User    string    `json:"user"`
	Message string    `json:"message"`
	Created time.Time `json:"created"`
	Tag     string    `json:"tag"`
}

func main() {
	esURL := "http://localhost:9200"
	ctx := context.Background()
	client, err := elastic.NewClient(
		elastic.SetURL(esURL),
		elastic.SetSniff(false),
	)
	if err != nil {
		panic(err)
	}

	//messageに「テスト」が含まれるドキュメントを検索
	matchQuery := elastic.NewMatchQuery("message", "テスト")
	results, err := client.Scroll("chat").Query(matchQuery).Size(1).Do(ctx)
	if err != nil {
		panic(err)
	}

	var chatType Chat
	for _, chat := range results.Each(reflect.TypeOf(chatType)) {
		if c, ok := chat.(Chat); ok {
			fmt.Printf("Chat message is: %s \n", c.Message)
		}
	}

	//さきほどの検索結果からスクロールIDを取得し、前回検索結果の続きからを取得
	nextResults, err := client.Scroll("chat").Query(matchQuery).Size(1).ScrollId(results.ScrollId).Do(ctx)
	if err != nil {
		panic(err)
	}

	for _, chat := range nextResults.Each(reflect.TypeOf(chatType)) {
		if c, ok := chat.(Chat); ok {
			fmt.Printf("Scrolled message is: %s \n", c.Message)
		}
	}
}

//}


実行してみるとmessageに「テスト」を含む2つのドキュメントがヒットしますが、スクロールAPIを利用しSize(1)で取得しているため、次のように出力されます。

//cmd{
# go run term_query.go
Chat message is: 明日は期末テストがあるけどなんにも勉強してない....
Scrolled message is: あと十年あれば期末テストもきっと満点がとれたんだろうな
//}


=== Multi Fields


Multi Fields機能を利用することで一つのフィールドに対して異なるデータ型やAnalyze設定を指定することができます。
といってもすぐにピンとこないかもしれませんので、実際にMulti Fieldsの設定をしているMapping定義をみていきましょう。


//list[elasticsearch-list17][Multi Fieldsの設定がされているMapping定義例]{

{
  "mappings": {
    "_doc": {
      "properties": {
        "user": {
          "type": "text",
          "fields": {
            "raw": {
              "type": "keyword"
            }
          }
        }
      }
    }
  }
}

//}


userフィールドのtypeにmulti_fieldを指定しています。以下のようにフィールドを指定して操作することができます。

 * user： type textが適用されているuserフィールドにアクセスします
 * user.keyword：type keywordが適用されうるフィールドにアクセスします

ドキュメントを登録する際にはこれまで通りuserフィールドを明示して登録するだけでよいです。

例えばMatchQueryの場合だと以下のようになります。


//list[elasticsearch-list177][user(type text)に対する検索]{
/* 省略 */

//「テスト」が含まれるドキュメントがヒット
query := elastic.NewMatchQuery("message", "テスト")
//}


//list[elasticsearch-list177][user(type keyword)に対する検索]{
/* 省略 */

//「テスト」に完全一致するドキュメントがヒット
query := elastic.NewMatchQuery("message.keyword", "テスト")
//}


=== エラーハンドリング


最後に、エラーハンドリングについて記載します。
@<code>{Elastic:An Elasticsearch client for the Go}では@<code>{elastic.Error}経由で詳細なエラー情報を取得できます。これをもとにしてエラーハンドリングを実装することができます。


//list[elasticsearch-list19][エラーハンドリング]{
 err := client.IndexExists("chat").Do()
if err != nil {
    // *elastic.Errorかどうかを判別
    e, ok := err.(*elastic.Error)
    if !ok {
        //エラーハンドリングを記載
    }
    log.Printf("status %d ,error %s.", e.Status, e.Details)
}
//}
