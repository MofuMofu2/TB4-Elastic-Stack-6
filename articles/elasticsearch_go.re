
= GoではじめるElasticsearch

== はじめに


Elasticsearchの入門の多くはREST APIを使ったものが多いですが、実際にアプリケーション作成する際は何らかの言語のSDKを利用するかと思います。
そうした際に意外と「あれ、これってどうやるんだ？」となる場合が多いものです。

そこで、本章ではElasticsearchの基本操作をGo言語を利用して体験していきます。Elasticsearchの基本的な操作を中心に、ちょっとしたTipsについても触れていきます。
Elasticsearchはとても多くの機能を有しています。そのため、全ての機能をカバーすることは難しいです。よって、代表的な機能について本章では記載します。 また本章ではElasticsearchのAPIを主に扱います。Elasticsearchのクラスタリング機能などについては、最低限の情報しか記載していません	。


== Elasticsearch環境の準備


今回はElastic社が提供している公式Dockerイメージを利用します。
下記のコマンドを実行してDockerイメージを取得してください。


//list[elasticsearch-list01][Dockerイメージの取得]{
docker pull docker.elastic.co/elasticsearch/elasticsearch:6.2.2
//}


Dockerイメージが起動できるかを確認します。


//list[elasticsearch-list02][Dockerイメージの起動]{
docker run -p 9200:9200  -e "discovery.type=single-node" -e "network.publish_host=localhost"
（紙面の都合により改行）docker.elastic.co/elasticsearch/elasticsearch:6.2.2
//}


起動に成功すると、プロンプト上に起動ログが出力されます。
ポートマッピングで指定している9200ポートはElasticsearchへのAPIを実行するためのエンドポイントです。
Elastic社のDockerイメージを利用すると、Docker起動時に環境変数経由で設定を変更できます。
起動時にいくつかオプションを指定しているため解説します。

まず、オプション@<code>{discovery.type}を@<code>{single-node}に設定しています。
このElasticsearchはクラスタを構成せず、シングルノード構成であることを明示します。すると、起動時に自分自身をマスタノードとして設定し起動します。

次に、@<code>{network.publish_host}を@<code>{loccalhost}に設定しました。
ElasticsearchのAPIエンドポイントとして公開するIPアドレスを指定します。
指定しなかった場合、Dockerコンテナ内部のプライベートIPアドレスになります。
ローカルホストから直接エンドポイントへ接続することができないため、この設定を入れています。


Dockerが正常に起動しているか確認してみましょう。さきほどマッピングした9200ポートでElasticsearchはREST APIのエンドポイントを公開しています。
@<list>{elasticsearch-list03}を用いてElasticsearchの基本情報について取得できるか確認してください。


//list[elasticsearch-list03][Elasticsearchの起動確認]{
curl http://localhost:9200
//}

//cmd{
# curl http://localhost:9200
{
  "name" : "TiaRqEF",
  "cluster_name" : "docker-cluster",
  "cluster_uuid" : "JMepnQSwQaGmI3fStZ_YTA",
  "version" : {
    "number" : "6.0.0",
    "build_hash" : "8f0685b",
    "build_date" : "2017-11-10T18:41:22.859Z",
    "build_snapshot" : false,
    "lucene_version" : "7.0.1",
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

それではクライアントをインストールしましょう。
今回はgo getでインストールしますが、実際のプロダクト利用時はdepなどのパッケージ管理ツールの利用をおすすめします。


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



と、このようにRDBMSで例えられることが多いのですが、ここで注意が必要なのがTypeはElasticsearch 7系より廃止が予定されています。
また5系までは1つのIndexに複数のTypeを作成できたのですが、6系では1つのIndexに1つのTypeのみ作成できる仕様へ変わっています
（参考：@<href>{https://www.elastic.co/guide/en/elasticsearch/reference/master/removal-of-types.html}）。

本章ではElasticsearch6系を利用するため、1 Indexに1 Typeを作成します。
また、ElasticsearchはMapping定義を作成しなくてもデータを投入することもできます。

その際は投入したJSONデータにあわせたMappingが自動で作成されます。
実際のアプリケーションでElasticsearchを利用する場合、Mapping定義によりデータスキーマを固めて利用することの方が多いかと思います。
また、Mapping定義を作成することにより、各フィールド単位でより細かな検索設定をおこなうことが可能です。本章では動的Mappingは利用せず、Mapping定義を1から作成し利用します。


=== Mapping


本章ではChatアプリケーションを想定したIndex/Typeをもとに操作をおこなっていきます。

Elasticsearchを操作するにあたり利用するMapping定義を@<list>{elasticsearch-list05}に記述しました。


//list[elasticsearch-list05][Mapping定義]{
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
        "tags": {
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
後ほど詳細を説明しますが、アナライザを適用することでそのフィールドに対し高度な全文検索を行うことができます。一方でkeyword型はアナライザが適用されないため、完全一致での検索が求めらます。
また、フィールドに対してソートをおこないたい場合はkeyword型を指定する必要があります。


Elasticsearch 6系のデータ型の詳細は公式ドキュメント（@<href>{https://www.elastic.co/guide/en/elasticsearch/reference/current/mapping-types.html}）を参照してみてください。
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


作成されたIndexを確認してみます。下記のエンドポイントから指定したIndex/TypeのMapping定義を確認できます。


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
          "tags" : {                                                                                      
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


それではGoを使ってElasticsearchを触っていきましょう。
まずはさきほどDockerで起動したElasticsearchへの接続確認も兼ねて、Elasticsearchのバージョン情報などを取得してみましょう。


//list[elasticsearch-list07][Go言語を用いてElasticsearchに接続する]{
package main

import (
    "context"
    "fmt"

    "github.com/olivere/elastic"
)

func main() {
    esUrl := "http://localhost:9200"
    ctx := context.Background()

    client, err := elastic.NewClient(
        elastic.SetURL(esUrl),
    )
    if err != nil {
        panic(err)
    }

    info, code, err := client.Ping(esUrl).Do(ctx)
    fmt.Printf("Elasticsearch returned with code %d and version %s\n", code, info.Version.Number)

}
//}


@<code>{elastic.NewClient}でクライアントを作成します。その際に@<code>{elastic.ClientOptionFunc}で複数の設定を引数とすることが可能です。
@<list>{elasticsearch-list07}では@<code>{elastic.SetURL()}にて接続する先のElasticsearchのエンドポイントを指定しています。
クライアントを作成すると、そのオブジェクトを通じてElasticsearchを操作することができるようになります。
Elasticsearchのバージョン情報といったシステム情報を取得する際は@<code>{Ping}を利用します。


では実行してみましょう!


//list[elasticsearch-list08][Elasticsearchのバージョン情報を問い合わせる]{
$ go run hello_elasticsearch.go
Elasticsearch returned with code 200 and version 6.0.0
//}


ローカル環境で稼働させているElasticsearchのバージョンが表示されれば無事接続成功です。


=== 単純なCRUD操作


それでは先ほど作成したIndexを対象に基本的なCRUD操作をおこなってみましょう。
操作を始めるために、まずはクライアントのオブジェクトを作成します。



このクライアントオブジェクトを通じてElasticsearchを操作していきます。
クライアントの作成時に以下の2つのオプションを指定しています。
特にSetSniffはElasticsearchのコンテナへ接続する際に必要となる設定です。



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

//list[elasticesearch-list08][クライアント側でドキュメントIDを付与する]{
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

const (
    ChatIndex = "Chat"
)

func main() {
    esUrl := "http://localhost:9200"
    ctx := context.Background()

    client, err := elastic.NewClient(
        elastic.SetURL(esUrl),
    )
    if err != nil {
        panic(err)
    }

    chatData := Chat{
        User:    "user01",
        Message: "test message",
        Created: time.Now(),
        Tag:     "tag01",
    }

    _, err = client.Index().Index("chat").Type("chat").Id("1").BodyJson(&chatData).Do(ctx)
    if err != nil {
        panic(err)
    }
}
//}

==== ドキュメントIDによる取得


次に先ほど登録したドキュメントを、@<code>{ドキュメントID}を指定して取得します。
@<code>{Elastic:An Elasticsearch client for the Go}では取得したドキュメントはStrucrtに保存し直し、そのStructのフィールドを経由してデータを取得できます。


//list[elasticsearch-list09][ドキュメントの取得]{

type Chat struct {
  User string `json:"user"`,
  Message string `json:"message"`
  Created time.Time `json:"created"`
  Tag string `json:"tag"`
}

func main() {
  esEndpoint := "http://localhost:9200"
  ctx := context.Background()

  client, err := elastic.NewClient(
    elastic.SetURL(esEndpoint),
    elastic.SetSniff(false),
  )
  if err != nil {
    panic(err)
  }

//}

==== ドキュメントの削除


ドキュメントIDをもとに登録したドキュメントを削除してみます。


//list[elasticsearch-list10][ドキュメントの削除]{
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

const (
    ChatIndex = "Chat"
)

func main() {
    esUrl := "http://localhost:9200"
    ctx := context.Background()

    client, err := elastic.NewClient(
        elastic.SetURL(esUrl),
    )
    if err != nil {
        panic(err)
    }

    chatData := Chat{
        User:    "user01",
        Message: "test message",
        Created: time.Now(),
        Tag:     "tag01",
    }

  //登録
  //省略


  //参照
  //省略

  //削除
    _, err = client.Delete().Index("chat").Type("chat").Id("1").Do(ctx)
    if err != nil {
        panic(err)
    }
}
//}


== 検索の基本


さて、基本的なCRUDを通じてElasticsearchの基本をおさえたところで、いよいよ検索処理についてみていきましょう。
Elasticsearchは多くの検索機能をサポートしていますが、本章ではその中でも代表的な以下についてみていきます。
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


Analyzerは以下の要素から構成されています。これらを組み合わせることでより柔軟な検索のためのインデックスを作成できます。
 * Tokenizer
  ** ドキュメントをどうトークン分割するかを定義します。トークン分割には様々な方法があり、有名なものだと形態素解析やN-Gramなどがあります。Tokenizerにより分割されたトークンをもとに検索文字列との比較がおこなわれます。各Analyzerは1つのTokenizerを持つことができます。
 * Character filters
  ** Tokenizerによるトークン分割がされる前に施す処理を定義します。例えば検索文字列のゆらぎを吸収するために、アルファベットの大文字・小文字を全て小文字に変換したり、カタカナの全角・半角を全て半角に統一したりといった処理をトークン分割の前処理として実施します。
 * Token filters
  ** Tokenizerによるトークン分割がされた後に施す処理を定義します。例えば、形態素解析のように品詞をもとにトークン分割する場合、分割後のトークンから検索には不要と思われる助詞を取り除くといった処理が該当します。


//image[analyzer_basic][Analyzerの構成要素]{
//}


Tokenizerで形態素解析を用いた場合の例は以下のようになります。


//image[analyzer_sample][Analyzerの仕組み]{
//}


このようにTokenizerだけでなく任意のFiltersを組みわせることで、検索要件に適したAnalyzerを作成し適用することができます。
本書では日本語形態素解析プラグインであるKuromojiを利用しAnalyzerの設定をおこなっていきます。


==== Kuromojiプラグインの導入


Kuromojiプラグインは標準ではElasticsearchに内蔵されていないため追加でプラグインをインストールする必要があります。
稼働しているDockerのコンテナのコンテナIDを調べbashからプラグインのインストールをおこなっていきましょう。
Elasticsearchではプラグインをインストールする際には@<code>{elasticsearch-plugin}を利用します。
またプラグインを有効にするためにプラグインインストール後にコンテナの再起動をおこなってください。


//cmd{
# docker ps

CONTAINER ID        IMAGE                                                     COMMAND                  CREATED             STATUS              PORTS                                            NAMES
9a96bafde5bd        docker.elastic.co/elasticsearch/elasticsearch-oss:6.0.0   "/usr/local/bin/dock…"   2 hours ago         Up 2 hours          0.0.0.0:9200->9200/tcp, 0.0.0.0:9300->9300/tcp   agitated_haibt

# docker exec -it 66cec7c14657 bash
[root@9a96bafde5bd elasticsearch]# bin/elasticsearch-plugin install analysis-kuromoji
//}


==== MappingへのAnalyzerの適用


先ほど作成したMapping定義をもとにAnalyzerの設定を加えていきましょう。
Analyzerの設定は@<code>{settings}内でおこなっていきます。Analyzerを適用したいフィールドに@<code>{analyzer}を指定することで適用できます。


//list[elasticsearch-list11][Analyzerの設定]{

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
        "tags": {
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
分類  分類  説明
-------------------------------------------------------------
Character Filters   kuromoji_iteration_mark 踊り字を正規化します。e.g) すゝめ→すすめ
Tokenizer   kuromoji_tokenizer  日本語での形態素解析により文章をトークン化します。
Token Filters   kuromoji_baseform 動詞など活用になりかわる言葉を原形にします。e.g) 読め→読む
Token Filters   kuromoji_part_of_speech 検索時には利用されないような助詞などの品詞を削除します。
Token Filters   ja_stop 文章中に頻出するあるいは検索で利用されることがない言葉を削除します。e.g) あれ、それ
Token Filters   kuromoji_number 漢数字を数字に変更します。e.g) 五->5
Token Filters   kuromoji_stemmer  単語の末尾につく長音を削除します。e.g) サーバー→サーバ
//}


作成したAnalyzerを適用したいMappingフィールドに指定することで、そのフィールドにAnalyzerで指定したインデクシングを施すことができます。
Chatマッピングの１階層下に存在する、messageフィールドのanalyzerにさきほど作成したAnalyzerを指定することで適用します。



ここではMapping定義を再作成します。


//list[elasticsearch-list12][Mapping定義の再作成]{
# curl -XDELETE 'http://localhost:9200/chat'
# curl -XPUT 'http://localhost:9200/chat' -H "Content-Type: application/json" -d @mapping.json
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

const (
    ChatIndex = "Chat"
)

func main() {
    esUrl := "http://localhost:9200"
    ctx := context.Background()

    client, err := elastic.NewClient(
        elastic.SetURL(esUrl),
    )
    if err != nil {
        panic(err)
    }

    chatData01 := Chat{
        User:    "user01",
        Message: "明日は期末テストがあるけどなんにも勉強してない....",
        Created: time.Now(),
        Tag:     "tag01",
    }
    
    chatData02 := Chat{
        User:    "user02",
        Message: "時々だけど勉強のやる気が出るけど長続きしない",
        Created: time.Now(),
        Tag:     "tag01",
    }

    chatData03 := Chat{
        User:    "user03",
        Message: "あと十年あれば期末テストもきっと満点がとれたんだろうな",
        Created: time.Now(),
        Tag:     "tag01",
    }

    chatData04 := Chat{
        User:    "user04",
        Message: "ドラえもんの映画で一番すきなのは夢幻三剣士だな",
        Created: time.Now(),
        Tag:     "tag01",
    }

    chatData05 := Chat{
        User:    "user05",
        Message: "世界記憶の概念、そうアカシックレコードを紐解くことで解は導かれるのかもしれない",
        Created: time.Now(),
        Tag:     "tag01",
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


MatchQueryは全文検索の肝です。MatchQueryでは、指定した検索文字列がAnalyzerにより言語処理がなされ検索がおこなわれます。
olivere/elasticで検索機能を利用する際は、client経由でSearchメソッドを実行します。
Searchメソッドはelastic.SearchServiceのQueryメソッドに検索条件を指定したelastic.MatchQueryを渡します。
取得できたドキュメントをStruct経由で操作する際はreflectパッケージを使って操作します。


//list[elasticsearch-list13][Searchメソッドの実行]{
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

const (
    ChatIndex = "Chat"
)

func main() {
    esUrl := "http://localhost:9200"
    ctx := context.Background()

    client, err := elastic.NewClient(
        elastic.SetURL(esUrl),
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
            fmt.Println("Chat message is: %s", c.Message)
        }
    }
}

//}


実行すると以下の２つのドキュメントがヒットします。(ファイル名をmain.goとして保存しています。)


//cmd{
# go run main.go
Chat message is: あと十年あれば期末テストもきっと満点がとれたんだろうな
Chat message is: 明日は期末テストがあるけどなんにも勉強してない....
//}


狙ったとおりのドキュメントを取得できました！では、この検索結果はどのように導かれたのでしょうか。
AnalyzerこれらのドキュメントがどのようにAnalyzeされインデクシングされているのか確認してみます。



//list[elasticsearch-list033][analyze api]{
curl -XPOST "http://localhost:9200/<Index名>/_analyze?pretty" -H "Content-Type: application/json" -d
  '{
    "analyzer": "Analyzer名",
    "text": "Analyzeしたい文字列"
  }'
//}

//cmd{

curl -XPOST "http://localhost:9200/chat/_analyze?pretty" -H "Content-Type: application/json" -d '{"analyzer": "kuromoji_analyzer", "text": "あと十年あれば期
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
Elastic:An Elasticsearch client for the GoでTermQueryを利用する際はTerm Queryは@<code>{elastic.TermQuery}を利用します。
elastic.NewTermQueryは検索対象のフィールドと検索文字列を指定します。


//list[elasticsearch-list14][Term Queryを用いたドキュメント検索]{
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

const (
    ChatIndex = "Chat"
)

func main() {
    esUrl := "http://localhost:9200"
    ctx := context.Background()

    client, err := elastic.NewClient(
        elastic.SetURL(esUrl),
    )
    if err != nil {
        panic(err)
    }

    termQuery := elastic.NewTermQuery("User", "山田")
    results, err := client.Search().Index("chat").Query(termQuery).Do(ctx)
    if err != nil {
        panic(err)
    }

    var chattype Chat
    for _, chat := range results.Each(reflect.TypeOf(chattype)) {
        if c, ok := chat.(Chat); ok {
            fmt.Println("Chat message is: %s", c.Message)
        }
    }

}
//}


=== Bool Query


BoolQueryではAND/OR/NOTによる検索がおこなえます。検索条件をネストさせることも可能で、より複雑な検索Queryを組み立てることができます。
実際にはmust/should/must_notといったElasticsearch独自の指定方法を利用します。

//table[tbl3][]{
Query	説明	oliver/elasticでの指定方法
-----------------
must	ANDに相当	boolQuery := elastic.NewBoolQuery() <br> boolQuery.Must(elastic.NewTermQuery("field", "value")
should	ORに相当	boolQuery := elastic.NewBoolQuery() <br> boolQuery.Should(elastic.NewTermQuery("field", "value")
must_not	NOT	boolQuery := elastic.NewBoolQuery() <br> boolQuery.MustNot(elastic.NewTermQuery("field", "value")
//}


userが「佐藤」で、messageに「Elasticsearch」が含まれるが「Solor」が含まれないドキュメントを検索するクエリは以下の通りです。


//list[elasticsearch-list15][Analyzerの設定]{
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

const (
    ChatIndex = "Chat"
)

func main() {
    esUrl := "http://localhost:9200"
    ctx := context.Background()

    client, err := elastic.NewClient(
        elastic.SetURL(esUrl),
    )
    if err != nil {
        panic(err)
    }

    boolQuery := elastic.NewBoolQuery()
    boolQuery.Must(elastic.NewTermQuery("user", "佐藤")
    boolQuery.Should(elastic.NewTermQuery("message", "Elasticsearch")
    boolQuery.MustNot(elastic.NewTermQuery("message", "Solor")
    results, err := client.Search().Index("chat").Query(termQuery).Do(ctx)
    if err != nil {
        panic(err)
    }

    var chattype Chat
    for _, chat := range results.Each(reflect.TypeOf(chattype)) {
        if c, ok := chat.(Chat); ok {
            fmt.Println("Chat message is: %s", c.Message)
        }
    }

}
//}


#@#//TODO:ネストがふかいもの


== ちょっと応用


ここでは少し応用的な機能についてみていきましょう。

 * Scroll API
 ** Elasticsearchが提供しているページング機能です。limit&offsetと違い、検索時のスナップショットを保持し、カーソルを利用してページの取得をおこなえます。
 * Multi Fields
 ** Multi Fieldsタイプを指定することで1つのフィールドに対してデータ型やAnalyze設定が異なる複数のフィールドを保持することができます。
 * Alias
 ** インデックスに別名をつけてアクセスすることができる機能です。任意の検索条件を指定したエイリアスも作成することが可能で、RDBのビューのような機能も利用できます。


=== Scroll API


Scroll APIを利用することで、スクロールタイプのページング機能を手軽に利用することができます。Elasticsearchでは@<code>{limit&offset}を用いた値の取得もできます。
ただし、@<code>{limit&offset}を利用した場合、検索がおこなわれる度に指定したoffsetからlimit数分のドキュメントを取得します。そのため、取得結果に抜け漏れや重複が生じる可能性があります。
一方でScroll APIを利用した場合、初回検索時にスナップショットが生成されます。そのため、Scroll APIが返すスクロールIDを利用することで、初回検索時のスナップショットに対して任意の箇所からページングをおこなうことができます。
使い方はとても簡単で、@<code>{elastic.ScrollService}を介して操作することができます。


//list[elasticsearch-list16][Scroll APIを利用した検索]{
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

const (
    ChatIndex = "Chat"
)

func main() {
    esUrl := "http://localhost:9200"
    ctx := context.Background()

    client, err := elastic.NewClient(
        elastic.SetURL(esUrl),
    )
    if err != nil {
        panic(err)
    }

    termQuery := elastic.NewTermQuery("user", "山田")
    results, err := client.Scroll("chat").Query(termQuery).Size(10).Do(ctx)
    if err != nil {
        panic(err)
    }

    results, err = client.Scroll("chat").Query(termQuery).Size(10).ScrollId(results.ScrollId).Do(ctx)
    if err != nil {
        panic(err)
    }
}
//}

=== Multi Fields


Multi Fields機能を利用することで一つのフィールドに対して異なるデータ型やAnalyze設定を指定することができます。
といってもすぐにピンとこないかもしれませんので、実際にMulti Fieldsの設定をしているMapping定義をみてみましょう。


//list[elasticsearch-list17][Multi Fieldsの設定がされているMapping定義の参照]{

{
  "mappings": {
    "_doc": {
      "properties": {
        "user": {
          "type": "text",
          "fields": {
            "raw": {
              "type":  "keyword"
            }
          }
        }
      }
    }
  }
}

//}


userフィールドのtypeにmulti_fieldを指定しています。以下のようにフィールドを指定して操作することができます。
 * user：Analyzeされていない
 * user.analyzed：Analyzeされている



インデクシングする際はuserフィールドにのみ投入すればOKです。


==== Alias


AliasはIndexに別名をつけてアクセスすることができる機能です。任意の検索条件を指定したエイリアスも作成することが可能で、RDBのビューのような機能も利用できます。
@<code>{Elastic:An Elasticsearch client for the Go}ではAliasServiceを経由して操作することができます。


//list[elasticsearch-list18][AliasServiceを利用してIndexを操作]{
package main

import (
    "context"
    "time"

    "github.com/olivere/elastic"
)

const (
    ChatIndex = "Chat"
)

func main() {
    esUrl := "http://localhost:9200"
    ctx := context.Background()

    client, err := elastic.NewClient(
        elastic.SetURL(esUrl),
    )
    if err != nil {
        panic(err)
    }

    termQuery := elastic.NewTermQuery("user", "山田")

  //chatインデックスに対してchat-aliasインデックスを作成
    client.Alias().Add("chat", "chat-alias").Do(ctx)
  //chatインデックスのmessageに山田だけが含まれるcaht-yamada-message-onlyエイリアスを作成
    client.Alias().AddWithFilter("chat", "chat-yamada-message-only", termQuery).Do(ctx)
  //chat-aliasエイリアスを削除
    client.Alias().Remove("chat", "chat-alias").Do(ctx)
}
//}


=== Synonym


=== エラーハンドリング


最後にエラーハンドリングについて記載します。
@<code>{Elastic:An Elasticsearch client for the Go}では@<code>{elastic.Error}経由で詳細なエラー情報を取得できます。これをもとにしてエラーハンドリングを実装することができます。


//list[elasticsearch-list19][エラーハンドリング]{
 err := client.IndexExists("chat").Do()
if err != nil {
    // Get *elastic.Error which contains additional information
    e, ok := err.(*elastic.Error)
    if !ok {
        //...
    }
    log.Printf("Elastic failed with status %d and error %s.", e.Status, e.Details)
}
//}
