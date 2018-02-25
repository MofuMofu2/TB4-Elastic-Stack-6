# GoではじめるElasticsearch

## はじめに
本章ではElasticsearchの基本操作をGo言語を通じて体験していきましょう。
基本的な操作を中心にちょっとしたTipsについても触れていきます。

## Elasticsearch環境の準備
今回はElastic社が提供しているDockerイメージを利用します。
下記のコマンドを実行してDockerイメージを取得します。

```
# docker pull elasticsearch
```

次にDockerイメージを起動します。

```
# docker run -p 9200:9200 -p 9300:9300 -e "discovery.type=single-node" -e "http.host=0.0.0.0" -e "transport.host=127.0.0.1" docker.elastic.co/elasticsearch/elasticsearch
```

起動に成功するとプロンプト上に起動ログが出力されます。

正常に起動しているか確認してみます。下記コマンドによりElasticsearchの基本情報について取得できるか確認してみてください。

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

>c https://hub.docker.com/_/elasticsearch/

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
### IndexとType
Elasticsearchで検索をおこなうために、まずIndexとTypeを作成します。
RDBMSで例えと以下に相当します。
* Indexはスキーマ/データベース
* Typeはテーブル

Typeにデータを登録していきます。ここで注意が必要なのが、TypeはElasticsearch 7系より廃止が予定されています。
また5系までは1つのIndexに複数のTypeを作成できたのですが、6系では1つのIndexに1つのTypeのみ作れる仕様へ変わっています。

> https://www.elastic.co/guide/en/elasticsearch/reference/master/removal-of-types.html

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

### 単純なCRUD操作
それでは先ほど作成したIndexを対象に基本的なCRUDE操作をおこなってみましょう。
操作を始めるために、まずはクライアントのオブジェクトを作成します。

```Go

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
}

```

このクライアントオブジェクトを通じてElasticsearchを操作していきます。
クライアントの作成時に以下の2つのオプションを指定しています。
特にSetSniffはElasticsearchのコンテナへ接続する際に必要となる設定です。

操作にあたっては、さきほど作成したMappingに対応するStructを通じておこなっていきます。
なので、今回サンプルとして利用するChat Mappingに対応するStructを定義します。

```
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
}
```

#### ドキュメントの登録
まずは単一のドキュメントを登録します。
Elasticsearchは登録されたドキュメントに対してドキュメントIDと呼ばれるドキュメントを一意に識別するためのIDとを付与します。
IDの振り方には登録時にクライアント側で設定するか、Elasticsearch側でランダムに振ってもらうかの2通りがあります。
今回は登録時にクライアント側でドキュメントIDを指定してみましょう。
さきほど作成したクライアントセッションを利用して操作をおこなっていきましょう。

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

### 検索の基本操作
さて、基本的なCRUTを通じてElasticsearchの基本をおさえたところで、いよいよ検索処理について見ていきましょう。
Elasticsearchは多くの検索機能をサポートしています。本章ではその中でも代表的な以下についてみていきます。

* Term Query
* Bool Query
* Query String Query

#### Term Query

#### Bool Query

#### Query String Query

### ちょっと応用
ここでは少し応用的な機能についてみていきましょう。

* Scroll
* マルチフィールド
* xxxxxx

#### Scroll

#### Multi Fieled

#### xxxxx

## エラーハンドリング

## Amazon Elasticsearch Serviceを利用する際のポイント

