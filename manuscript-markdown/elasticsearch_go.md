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
### IndexとType
Elasticsearchで検索をおこなうために、まずIndexとTypeを作成しましょう。
RDBMSで例えると以下に相当します。
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
          "type": "keyword"
        },
        "created": {
          "type": "date"
        },
        "tags": {
          "type": "text"
        }
      }
    }
  }
}
```

//TODO: Mappingの項目説明を加える

### 単純なCRUD操作

### 検索の基本操作

### ちょっと応用操作

## エラーハンドリング

## Amazon Elasticsearch Serviceを利用する際のポイント

