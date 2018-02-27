# GoではじめるElasticsearch

## はじめに
Elasticsearchの入門の多くはREST APIを使用しています。しかし、実際にアプリケーションを作成する際は何らかの言語のSDKを利用するかと思います。
<!-- Mofu SDKは言葉の定義をした方が良いです（例をあげるなど）。複数解釈は排除した方がよいので…。 -->
すると、「あれ、これってどうやるんだ？」と操作に迷う場面も増えるでしょう。


そこで、本章ではGo言語を通じてElasticsearchの基本操作を体験していきます。基本的な操作を中心に、ちょっとしたTipsについても触れます。

Elasticsearchはとても多くの機能を有しているので、代表的な機能について本章では取り扱います。
またElasticsearchのAPIを主に扱うため、Elasticsearchのクラスタリング機能といった機能は最低限の解説を行います。

## Elasticsearch環境の準備
今回はElastic社が提供している公式Dockerイメージを利用します。
下記のコマンドを実行してDockerイメージを取得してください。

<!-- Mofu ローカル環境の前提（Dockerはインストール済みとします）・実際にこれ以降の操作を行った環境（Docker for Mac利用、MacOSのバージョンなど）を記述した方が良いです。WinかLinux/Macかでだいぶ操作感が異なるので -->

```
# docker pull docker.elastic.co/elasticsearch/elasticsearch:6.2.2
```

下記コマンドで取得したDockerイメージが起動できるかを確認します。

```
# docker run -p 9200:9200  -e "discovery.type=single-node" -e "network.publish_host=localhost" docker.elastic.co/elasticsearch/elasticsearch:6.2.2
```

起動に成功するとプロンプト上に起動ログが出力されます。
<!-- Mofu ここは標準出力結果があるとわかりやすいです -->

ポートマッピングで指定している9200ポートはElasticsearchへのAPIを実行するためのエンドポイントです。

Elastic社のDockerイメージは、Docker起動時に環境変数経由で設定を変更できます。
本章で指定しているオプションは以下の通りです。


<!-- Mofu ここは表を作らず、文章で解説した方がわかりやすいと思います。表を作るなら列はオプション・値のみが良いでしょう。 -->

| オプション           | 値          | 説明                                                                                                                                                                                                                                                 |
|----------------------|-------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| discovery.type       | single-node | このElasticsearchはクラスタを構成せず、シングルノード構成であることを明示します。そうすることで起動時に自分自信をマスタノードとして設定し起動します。                                                                                                |
| network.publish_host | localhost   | ElasticsearchのAPIエンドポイントとして公開するIPアドレスを指定します。指定しなかった場合、Dockerコンテナ内部のプライベートIPアドレスになります。そのため、ローカルホストから直接エンドポイントへ接続することができないため、この設定を入れています。 |

正常に起動しているか確認してみましょう。さきほどマッピングした9200ポートでElasticsearchはREST APIのエンドポイントを公開しています。
下記コマンドを使用して、Elasticsearchの動作確認を行います。curlコマンドを基本情報について取得できるか確認してみてください。

<!-- Mofu Dockerコンテナにログインしてからcurlコマンドを発行しますか？もしそうであれば、その手順も含めて記述するとよりわかりやすいです。 -->

```
# curl http://localhost:9200
{                                              
  "name" : "WlZn3XP",                          
  "cluster_name" : "docker-cluster",           
  "cluster_uuid" : "7Ltq7Ph_Tv-cLofAglwp_g",   
  "version" : {                                
    "number" : "5.6.4",                        
    "build_hash" : "8bbedf5",                  
    "build_date" : "2017-10-31T18:55:38.105Z", 
    "build_snapshot" : false,                  
    "lucene_version" : "6.6.1"                 
  },                                           
  "tagline" : "You Know, for Search"           
}
```

ElasticsearchのDockerイメージの細かなオプションなどは下記に記載があります。

> https://hub.docker.com/_/elasticsearch/

## クライアントライブラリの選定
まずはElasticsearchを操作するためのクライアントライブラリを決める必要があります。
Elastic社の公式クライアントもあるのですが、現時点では絶賛開発中なうえにあまり活発にメンテナンスもされていません。。。

> https://github.com/elastic/go-elasticsearch

今回は下記のクライアントを利用します。

> https://github.com/olivere/elastic

こちらのクライアントは開発も活発で、Elasticの早いバージョンアップにもいち早く対応されてます。

それではライブラリをインストールしてみましょう。
今回はgo getでインストールしますが、実際のプロダクト利用時はdepなどのパッケージ管理ツールの利用をおすすめします。

```
# go get "github.com/olivere/elastic"
```

## Goで始めるElasticsearch
さて、いよいよGoでElasticsearchを操作していきましょう。
しかしその前に検索するデータを投入するためのIndexとTypeを作成していきましょう。

### IndexとType
Elasticsearchで検索をおこなうために、まずIndexとTypeを作成する必要があります。
RDBMSで例えと以下に相当します。
* Indexはスキーマ/データベース
* Typeはテーブル

と、よくこのようにRDBMSで例えられることが多いのですが、ここで注意が必要なのがTypeはElasticsearch 7系より廃止が予定されています。
また5系までは1つのIndexに複数のTypeを作成できたのですが、6系では1つのIndexに1つのTypeのみ作れる仕様へ変わっています。

> https://www.elastic.co/guide/en/elasticsearch/reference/master/removal-of-types.html

本書ではElasticsearch6系を利用するため、1 Indexに1 Typeを作成します。
また、ElasticsearchはMapping定義を作成しなくてもデータを投入することもできます。
その際は投入したJSONデータにあわせたMappingが自動で作成されます。
実際のアプリケーションでElasticsearchを利用する場合、Mapping定義によりデータスキーマを固めて利用することの方が多いかと思います。
また、Mapping定義を作成することにより、各フィールド単位でより細かな検索設定をおこなえるため、本章では動的Mappingは利用せず、Mapping定義を作成し利用します。

### 本章で利用するMapping定義
本書ではChatアプリケーションを想定したIndex/Typeをもとに操作をおこなっていきます。

Elasticsearchを操作するにあたり利用するMapping定義は以下の通りです。

```
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
```

今回はchatというTypeへドキュメントを登録していきます。propertiesにフィールドの項目を設定していきます。
フィールド名とそのデータ型を"tyep"で指定していきます。今回指定しているデータ型は以下の通りです。

| データ型 | 説明                                                                                                                                              |
|----------|---------------------------------------------------------------------------------------------------------------------------------------------------|
| keyword  | いわゆるString型です。後述するtext型もString型に相当します。しかしkeyword型の場合、そのフィールドへアナライザは適用されません。                   |
| text     | String型に相当します。text型を指定したフィールドはアナライザと呼ばれるElasticsearchの高度な検索機能を利用した検索が可能となります。               |
| date     | 日付型です。Elasticsearchへのデータ投入はJSONを介して行うため、実際にデータを投入する際はdateフォーマットに即した文字列を投入することになります。 |

keyword型とtext型は両者ともString型に相当します。その違いはアナライザを設定できるか否かです。
後ほど詳細を説明しますが、アナライザを適用することでそのフィールドに対し高度な全文検索を行うことができます。一方でkeyword型はアナライザが適用されないため、完全一致での検索が求めらます。

Elasticsearch 6系のデータ型の詳細は本家ドキュメントを参照してみてください。
多くのデータ型が標準でサポートされていています。

> https://www.elastic.co/guide/en/elasticsearch/reference/current/mapping-types.html

### Hello, Elasticsearch with GO
それでGoを使ってElasticsearchを触っていきましょう。
まずはさきほどDockerで起動したElasticsearchへの接続確認も確認も兼ねて、Elasticsearchのバージョン情報などを取得してみましょう。

```Go
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
```

elastic.NewClientでクライアントを作成します。その際にelastic.ClientOptionFuncで複数の設定を渡すことができます。
上のサンプルではelastic.SetURL()にて接続する先のElasticsearchのエンドポイントを指定しています。
クライアントを作成すると、そのオブジェクトを通じてElasticsearchを操作することができるようになります。
Elasticsearchのバージョン情報といったシステム情報を取得する際はPingを利用します。


### 単純なCRUD操作
それでは先ほど作成したIndexを対象に基本的なCRUDE操作をおこなってみましょう。
操作を始めるために、まずはクライアントのオブジェクトを作成します。


このクライアントオブジェクトを通じてElasticsearchを操作していきます。
クライアントの作成時に以下の2つのオプションを指定しています。
特にSetSniffはElasticsearchのコンテナへ接続する際に必要となる設定です。

操作にあたっては、さきほど作成したMappingに対応するStructを通じておこなっていきます。
なので、今回サンプルとして利用するChat Mappingに対応するStructを定義します。

```
type Chat struct {
	User    string    `json:"user"`
	Message string    `json:"message"`
	Created time.Time `json:"created"`
	Tag     string    `json:"tag"`
}
```

#### ドキュメントの登録
まずは単一のドキュメントを登録します。
Elasticsearchは登録されたドキュメントに対してドキュメントIDと呼ばれるドキュメントを一意に識別するためのIDとを付与します。
IDの振り方には登録時にクライアント側で設定するか、Elasticsearch側でランダムに振ってもらうかの2通りがあります。
今回は登録時にクライアント側でドキュメントIDを指定してみましょう。
さきほど作成したクライアントセッションを利用して操作をおこなっていきましょう。

```Go
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
```

#### ドキュメントIDによる取得
次に先ほど登録したドキュメントをドキュメントIDを指定して取得します。
olivere/elasticでは取得したドキュメントはStrucrtに詰め直し、そのStructのフィールドを経由してデータを取得できます。

```Go

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

```

#### ドキュメントの削除
ドキュメントIDをもとに登録したドキュメントを削除してみます。

```Go
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
```

### 検索の基本操作
さて、基本的なCRUTを通じてElasticsearchの基本をおさえたところで、いよいよ検索処理について見ていきましょう。
Elasticsearchは多くの検索機能をサポートしていますが、本章ではその中でも代表的な以下についてみていきます。

* Match Query
  * 指定した文字列での全文検索をおこないます。検索時に指定した文字列はAnalyzerにより言語処理がなされたうえで、検索がおこなれます。
* Term Query
  * 指定した文字列での検索をおこないますが、Match Queryとは違い検索指定文字列がAnalyzeされません。例えば、タグ検索のように指定した文字列で完全一致させたドキュメントを探したい時などはTerm Queryを利用するといったケースです。
* Bool Query
  * AND/OR/NOTによる検索がおこなえます。実際にはmust/should/must_notといったElasticsearch独自の指定方法を利用します。検索条件をネストさせることも可能で、より複雑な検索クエリを組み立てることができます。

#### Match Query
Matchクエリは全文検索の肝です。Matchクエリでは、指定した検索文字列がAnalyzerにより言語処理がなされ検索がおこなわれます。
ここでAnalyzerについて簡単に説明します。Analyzerの設定は全文検索処理の要です。そのため、設定内容も盛り沢山ですし、自然言語処理の知識も必要となってくるため、ここではあくまで触りだけを説明します。
この本をきっかけにElasticsearchにもっと興味を持っていただけた方はAnalyzerを深掘ってみてください。

#### Analyzerの基本
Analyzerは以下の要素から構成されています。これらを組み合わせることでより柔軟な検索のためのインデックスを作成できます。
* Tokenizer
  * ドキュメントをどうトークン分割するかを定義します。トークン分割には様々な方法があり、有名なものだと形態素解析やN-Gramなどがあります。Tokenizerにより分割されたトークンをもとに検索文字列との比較がおこなわれます。各Analyzerは1つのTokenizerを持つことができます。
* Character filters
  * Tokenizerによるトークン分割がされる前に施す処理を定義します。例えば検索文字列のゆらぎを吸収するために、アルファベットの大文字・小文字を全て小文字に変換したり、カタカナの全角・半角を全て半角に統一したりといった処理をトークン分割の前処理として実施します。
* Token filters
  * Tokenizerによるトークン分割がされた後に施す処理を定義します。例えば、形態素解析のように品詞をもとにトークン分割するような場合、分割後のトークンから検索には不要そうな助詞を取り除いたりといった処理が該当します。

//TODO: イメージ図をいれる

ここでは先ほど作成したMapping定義をもとにAnalyzerの設定を加えてみます。
さきほどのChat Mappingのmessageフィールドに日本語形態素解析プラグインであるKuromojiを適用してみましょう。

```
{
  "settings": {
    "analysis": {
      "tokenizer": {
        "kuromoji_tokenizer_search": {
          "type": "kuromoji_tokenizer",
          "mode": "search",
          "discard_punctuation": "true"
        }
      },
      "analyzer": {
        "kuromoji_analyzer": {
          "type": "custom",
          "tokenizer": "kuromoji_tokenizer_search",
          "filter": [
            "kuromoji_baseform",
            "kuromoji_part_of_speech"
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
```

Analyzerの設定はMapping定義のanalysisでおこないます。tokenizerでトークン分割の方法を設定し、analyzerで設定したtoknenizerと各filter群を組み合わせて一つのAnalyzerを作ります。
本書では以下の設定でAnalyzerを設定しました。

* アナライザ名
  * kuromoji_analyzer
* 適用Filter
  * kuromoji_base: xxxxxxxxxxx
  * kuromoji_part_of_speech: xxxxxxxxxxx

作成したAnalyzerを適用したいMappingフィールドに指定することで、そのフィールドにAnalyzerで指定したインデクシングを施すことができます。
Chatマッピングのmessageフィールドのanalyzerにさきほど作成したAnalyzerを指定することで適用します。

ここではMapping定義を再作成します。

```
# curl -XDELETE 'http://localhost:9200/chat'
# curl -XPUT 'http://localhost:9200/chat' -H "Content-Type: application/json" -d @mapping.json
```

これで準備が整いました！それではここの詳細にうつっていきましょう。

#### Match Query
olivere/elasticで検索機能を利用する際は、client経由でSearchメソッドを実行します。
Searchメソッドはelastic.SearchServiceのQueryメソッドに検索条件を指定したelastic.MatchQueryを渡します。
取得できたドキュメントをStruct経由で操作する際はreflectパッケージを使って操作します。

```
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

```

#### Term Query
Termクエリを利用することで、指定した文字列を完全に含むドキュメントを検索することができます。
olivere/elasticでTermクエリを利用する際はTerm Queryはelastic.TermQueryを利用します。
elastic.NewTermQueryは検索対象のフィールドと検索文字列を指定します。

```
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
```

#### Bool Query
BoolクエリではAND/OR/NOTによる検索がおこなえます。検索条件をネストさせることも可能で、より複雑な検索クエリを組み立てることができます。
実際にはmust/should/must_notといったElasticsearch独自の指定方法を利用します。


| クエリ   | 説明              | oliver/elasticでの指定方法                                                                        |
|----------|-------------------|---------------------------------------------------------------------------------------------------|
| must     | ANDに相当します。 | boolQuery := elastic.NewBoolQuery() <br> boolQuery.Must(elastic.NewTermQuery("field", "value")    |
| should   | ORに相当します。  | boolQuery := elastic.NewBoolQuery() <br> boolQuery.Should(elastic.NewTermQuery("field", "value")  |
| must_not | NOTに相当します。 | boolQuery := elastic.NewBoolQuery() <br> boolQuery.MustNot(elastic.NewTermQuery("field", "value") |

userが「佐藤」で、messageに「Elasticsearch」が含まれるが「Solor」が含まれないドキュメントを検索するクエリは以下の通りです。

```
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
```

//TODO:ネストがふかいもの

### ちょっと応用
ここでは少し応用的な機能についてみていきましょう。

* Scroll API
  * Elasticsearchが提供しているページング機能です。limit&offsetと違い、検索時のスナップショットを保持し、カーソルを利用してページの取得をおこなえます。
* Multi Fields
  * Multi Fieldsタイプを指定することで1つのフィールドに対してデータ型やAnalyze設定がことなる複数のフィールドを保持することができます。
* Alias
  * インデックスに別名をつけてアクセスすることができる機能です。任意の検索条件を指定したエイリアスも作成することが可能で、RDBのビューのような機能も利用できます。

#### Scroll API
Scroll APIを利用することで、スクロールタイプのページング機能を手軽に利用することができます。Elasticsearchにはlimit&offsetでの取得もできます。
ただし、limit&offsetの場合、都度検索がおこなわれたうえで指定したoffsetからlimit数分のドキュメントを取得するため、取得結果に抜け漏れや重複が生じる可能性があります。
一方でScroll APIを利用した場合、初回検索時のスナップショットが生成されます。そのため、Scroll APIが返すスクロールIDを利用することで、初回検索時のスナップショットに対して任意の箇所からページングをおこなうことができます。
使い方もとても簡単で、elastic.ScrollServiceを介して操作することができます。

```
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
```

#### Multi Fields
Multi Fields機能を利用することで一つのフィールドに対してことなるデータ型やAnalyze設定を指定することができます。
といってもすぐにピンとこないかもしれませんので、実際にMulti Fieldsの設定をしているMapping定義をみてみましょう。

```

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

```

userフィールドのtypeにmulti_fieldを指定しています。以下のようにフィールドを指定して操作することができます。
* user：Analyzeされていない
* user.analyzed：Analyzeされている

インデクシングする際はuserフィールドにのみ投入すればOKです。

#### Alias
Aliasを利用することでインデックスに別名をつけてアクセスすることができる機能です。任意の検索条件を指定したエイリアスも作成することが可能で、RDBのビューのような機能も利用できます。
olivere/elasticではAliasServiceを経由して操作することができます。

```
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
```

## エラーハンドリング
最後にエラーハンドリングについて記載します。
olivere/elasticではelastic.Error経由で詳細なエラー情報を取得できます。これをもとにしてエラーハンドリングを実装することができます。

```Go
 err := client.IndexExists("chat").Do()
if err != nil {
    // Get *elastic.Error which contains additional information
    e, ok := err.(*elastic.Error)
    if !ok {
        //...
    }
    log.Printf("Elastic failed with status %d and error %s.", e.Status, e.Details)
}
```
