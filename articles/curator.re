﻿= Curatorを用いてIndexを操作する

== Curatorとは

Curatorは、Elasticsearchに保存したログのindex操作や、スナップショットの取得などを行う運用支援ツールです。
この章では、Elasticsearchに保存したindexの削除や、検索対象から外す方法について触れていきます。
Curatorの詳細は、こちらのリンク@<href>{https://www.elastic.co/guide/en/elasticsearch/client/curator/current/index.html}を参照してください。

まず、Curatorの実行環境を構築します。

=== Curatorのインストール

始めに、Curatorのパッケージを取得するため、リポジトリの登録をします。
ここで登録するリポジトリは、「AWSでLogstashを使ってみる」とは別のため登録が必要です。

//list[curator-01][curator.repoの追加]{
sudo vim /etc/yum.repos.d/curator.repo
//}

//cmd{
[curator-5]
name=CentOS/RHEL 6 repository for Elasticsearch Curator 5.x packages
baseurl=https://packages.elastic.co/curator/5/centos/6
gpgcheck=1
gpgkey=https://packages.elastic.co/GPG-KEY-elasticsearch
enabled=1
//}

Curatorをインストールします。


//list[curator-02][curatorのインストール]{
yum install elasticsearch-curator
//}


== indexの削除

ログ分析などの運用を行うと、大量のログデータが溜まっていきます。
ログを保存しているサーバのディスク容量を圧迫するので、結果としてパフォーマンス低下が発生する可能性があります。
そこでCuratorの登場です。
Curatorを使用すると、任意の期間を指定してindexを削除することが可能です。


補足ですが、「AWSでLogstashを使ってみる」や「Beatsを体験する」でElasticsearchに保存したログは、日ごとにindexが作成されていきます。
たとえば、2018年4月1日にLogstashがElasticsearchに保存したログは、@<code>{logstash-2018.04.01}というindexに保存されます。
2018年4月2日に保存したログは、@<code>{logstash-2018.04.02}というindexに保存されます。
100日経過した時には、100indexが作成されます。
このように時系列で作成していくindexを時系列indexと言います。


=== indexの削除操作

indexが2018年4月1日〜4月5日まであるとします。
curlコマンドを利用して、indexが存在することを確認します。

//list[curator-03][indexの確認]{
curl -XGET localhost:9200/_cat/indices/logstash* | sort
yellow open logstash-2018.04.01 5 1 8 0  93.2kb  93.2kb
yellow open logstash-2018.04.02 5 1 9 0 102.8kb 102.8kb
yellow open logstash-2018.04.03 5 1 9 0 102.8kb  72.8kb
yellow open logstash-2018.04.04 5 1 4 0  14.5kb  14.5kb
yellow open logstash-2018.04.05 5 1 4 0  14.5kb 104.5kb
//}

Curatorは、設定ファイルとアクションファイルの2つのファイルで構成されています。


設定ファイルの@<code>{curator.yml}を作成します。

//list[curator-04][curator.ymlの作成]{
vim ~/.curator/curator.yml
//}

//list[curator-05][curator.yml]{
---
client:
  hosts:
    - 127.0.0.1
  port: 9200
  url_prefix:
  use_ssl: False
  certificate:
  client_cert:
  client_key:
  ssl_no_validate: False
  http_auth:
  timeout: 30
  master_only: False

logging:
  loglevel: INFO
  logfile: '/var/log/curator'
  logformat: default
  blacklist: ['elasticsearch', 'urllib3']
//}

主な設定項目について表で説明します。

//table[curator-06][curator.ymlの設定項目]{
No.	Item	Content
-----------------
1	hosts	ElasticsearchのIPアドレスを指定
2	port	Elasticsearchのポートを指定
3	logfile	ログファイルの出力先の指定
//}

次にindex削除を定義したアクションファイルの@<code>{delete_indices.yml}を作成します。
今回は、1日分のindex保持させるため、unit_countを1に指定します。

//list[curator-07][delete_indices.ymlの作成]{
vim ~/.curator/delete_indices.yml
//}

//cmd{
---
actions:
  1:
    action: delete_indices
    description: delete logstash index
    options:
      ignore_empty_list: True
      disable_action: False
    filters:
    - filtertype: pattern
      kind: prefix
      value: logstash-
    - filtertype: age
      source: name
      direction: older
      timestring: '%Y.%m.%d'
      unit: days
      unit_count: 1
//}

主な設定項目について表で説明します。

//table[curator-08][delete_indices.ymlの設定項目]{
No.	Item	Content
-----------------
1	action	アクションを指定します（今回は、削除を指定）
2	filtertype	indexのフィルタ方式を指定します
3	pattern	任意のindexパターンを指定する場合に使用します
4	age	時間指定する場合に使用します
5	unit	時間単位を指定します
6	unit_count	時間を指定します（今回は、unitでdayを指定しているため、1日分を保持）
//}


index削除の環境が整ったので、Curatorを実行します。

まずCuratorのコマンドラインの引数について説明します。
オプションの--configは、@<code>{curator.yml}を@<code>{~/.curator/curator.yml}以外のディレクトリに配置した場合、使用します。
今回は、@<code>{~/.curator/curator.yml}に配置しているため、--configオプションは使用しません。

//list[curator-09][Curatorの実行引数]{
curator [--config CONFIG.YML] [--dry-run] delete_indices.yml
//}

それではDRY-RUNで実行します。
DRY-RUNを使用することで、設定ファイルに不備がないかを確認することができます。


//list[curator-10][CuratorをDRY-RUNで削除実行]{
curator --dry-run ~/.curator/delete_indices.yml
//}

@<code>{/var/log/curator}配下にCuratorの動作ログが出力されます。
DRY-RUNで実行した場合、ログにDRY-RUNと表記されます。
動作ログから、最新のindex以外は削除対象となったことがわかります。

//list[curator-11][ログの確認]{
cat /var/log/curator
//}

//cmd{
INFO      Preparing Action ID: 1, "delete_indices"
INFO      Trying Action ID: 1, "delete_indices": delete logstash index
INFO      DRY-RUN MODE.  No changes will be made.
INFO      (CLOSED) indices may be shown that may not be acted on by action "delete_indices".
INFO      DRY-RUN: delete_indices: logstash-2018.04.01 with arguments: {}
INFO      DRY-RUN: delete_indices: logstash-2018.04.02 with arguments: {}
INFO      DRY-RUN: delete_indices: logstash-2018.04.03 with arguments: {}
INFO      DRY-RUN: delete_indices: logstash-2018.04.04 with arguments: {}
INFO      Action ID: 1, "delete_indices" completed.
INFO      Job completed.
//}


次にDRY-RUNをオプションから外して実行します。

//list[curator-12][Curatorで削除実行]{
curator ~/.curator/delete_indices.yml
//}

Curator実行後、動作ログを確認します。

//list[curator-13][/var/log/curatorの確認結果]{
INFO      Preparing Action ID: 1, "delete_indices"
INFO      Trying Action ID: 1, "delete_indices": delete logstash index
INFO      Deleting selected indices: ['logstash-2018.04.10', 'logstash-2018.04.11']
INFO      DRY-RUN: delete_indices: logstash-2018.04.01 with arguments: {}
INFO      DRY-RUN: delete_indices: logstash-2018.04.02 with arguments: {}
INFO      DRY-RUN: delete_indices: logstash-2018.04.03 with arguments: {}
INFO      DRY-RUN: delete_indices: logstash-2018.04.04 with arguments: {}
INFO      Action ID: 1, "delete_indices" completed.
INFO      Job completed.
//}

最後に、curlコマンドでindexが削除されているか確認します。

//list[curator-14][indexの確認]{
curl -XGET localhost:9200/_cat/indices/logstash* | sort
//}

//cmd{
curl -XGET localhost:9200/_cat/indices/logstash* | sort
yellow open logstash-2018.04.05 5 1 4 0  14.5kb 104.5kb
//}

2018年4月5日のindexのみが保存されていることがわかります。


== indexのCloseとOpen

次は、indexのClose方法について説明します。
ログをElasticsearchに保存し続け、かつパフォーマンスは低下させたくないときにCloseを利用します。
indexをCloseすることで、サーバのメモリを解放します。これにより、サーバのパフォーマンスを維持できます。

Closeしたindexは、openを使用すれば再度データを閲覧することが可能です。

=== indexのClose


indexの削除を実施した時と同様、2018年4月1日〜4月5日までのindexがElasticsearchに保存されていると仮定します。


すでに@<code>{curator.yml}は作成済みなので、新しく@<code>{close_indices.yml}を作成します。

Closeするindexの対象は、最新のindex以外とします。

//list[curator-15][close_indices.ymlの作成]{
vim ~/.curator/close_indices.yml
//}

actionに@<code>{close}を指定し、indexをCloseします。

//list[curator-16][close_indices.yml]{
---
actions:
  1:
    action: close
    description: close logstash index
    options:
      delete_aliases: False
      disable_action: False
    filters:
    - filtertype: pattern
      kind: prefix
      value: logstash-
    - filtertype: age
      source: name
      direction: older
      timestring: '%Y.%m.%d'
      unit: days
      unit_count: 1
//}


DRY-RUNのオプションをつけて、Curatorを実行します。

//list[curator-17][CuratorをDRY-RUNでClose実行]{
curator --dry-run ~/.curator/close_indices.yml
//}

ログの実行結果から、Close対象のindexを確認します。


//list[curator-18][ログの確認]{
cat /var/log/curator
//}

//cmd{
INFO      Preparing Action ID: 1, "close"
INFO      Trying Action ID: 1, "close": close logstash index
INFO      DRY-RUN MODE.  No changes will be made.
INFO      (CLOSED) indices may be shown that may not be acted on by action "close".
INFO      DRY-RUN: close: logstash-2018.04.01 with arguments: {'delete_aliases': False}
INFO      DRY-RUN: close: logstash-2018.04.02 with arguments: {'delete_aliases': False}
INFO      DRY-RUN: close: logstash-2018.04.03 with arguments: {'delete_aliases': False}
INFO      DRY-RUN: close: logstash-2018.04.04 with arguments: {'delete_aliases': False}
INFO      Action ID: 1, "close" completed.
INFO      Job completed.
//}

次にDRY-RUNを外して実行します。


//list[curator-19][Curatorでclose実行]{
curator ~/.curator/close_indices.yml
//}

DRY-RUNで実行した時と同様にログを確認します。


//list[curator-20][ログの確認]{
INFO      Preparing Action ID: 1, "close"
INFO      Trying Action ID: 1, "close": close logstash index
INFO      Closing selected indices: ['logstash-2018.04.01', （紙面の都合により改行）
'logstash-2018.04.02', 'logstash-2018.04.03', 'logstash-2018.04.04']
INFO      Action ID: 1, "close" completed.
INFO      Job completed.
//}

curコマンドを利用して、indexが存在することを確認します。

//list[curator-21][indexの確認]{
curl -XGET localhost:9200/_cat/indices/logstash* | sort
//}

2018年4月5日以外のindexがCloseされました。

//cmd{
curl -XGET localhost:9200/_cat/indices/logstash* | sort

yellow close logstash-2018.04.01 5 1 8 0  93.2kb  93.2kb
yellow close logstash-2018.04.02 5 1 9 0 102.8kb 102.8kb
yellow close logstash-2018.04.03 5 1 9 0 102.8kb  72.8kb
yellow close logstash-2018.04.04 5 1 4 0  14.5kb  14.5kb
yellow open logstash-2018.04.05 5 1 4 0  14.5kb 104.5kb

//}


=== indexのOpen

@<list>{curator-21}でCloseしたindexをopenします。


@<code>{open_indices.yml}を作成します。
過去5日分を対象にopenするため、@<code>{unit_count}を5に設定します。
directionで新しい方から数えるのか、古い方から数えるのかを指定できます。
今回は、新しい方から数えるため、youngerを指定します（古い方からの場合、olderを指定してください）。

//list[curator-22][open_indices.ymlの作成]{
vim ~/.curator/open_indices.yml
//}

//list[curator-23][open_indices.yml]{
---
actions:
  1:
    action: open
    description: open logstash index
    options:
      disable_action: False
    filters:
    - filtertype: pattern
      kind: prefix
      value: logstash-
      exclude:
    - filtertype: age
      source: name
      direction: younger
      timestring: '%Y.%m.%d'
      unit: days
      unit_count: 5
//}

DRY-RUNオプションを利用して、Curatorを実行します。


//list[curator-24][CuratorをDRY-RUNでOpen実行]{
curator --dry-run ~/.curator/open_indices.yml
//}

DRY-RUNでログの実行結果を確認します。
open対象のindexが、ログの結果からわかります。


//list[curator-25][ログの確認]{
cat /var/log/curator
//}

//cmd{
2018-xx-xx xx:xx:xx,xx INFO      Preparing Action ID: 1, "open"
2018-xx-xx xx:xx:xx,xx INFO      Trying Action ID: 1, "open": open logstash index
2018-xx-xx xx:xx:xx,xx INFO      DRY-RUN MODE.  No changes will be made.
2018-xx-xx xx:xx:xx,xx INFO      (CLOSED) indices may be shown that may not be acted on by action "open".
2018-xx-xx xx:xx:xx,xx INFO      DRY-RUN: open: logstash-2018.04.01 (CLOSED) with arguments: {}
2018-xx-xx xx:xx:xx,xx INFO      DRY-RUN: open: logstash-2018.04.02 (CLOSED) with arguments: {}
2018-xx-xx xx:xx:xx,xx INFO      DRY-RUN: open: logstash-2018.04.03 (CLOSED) with arguments: {}
2018-xx-xx xx:xx:xx,xx INFO      DRY-RUN: open: logstash-2018.04.04 (CLOSED) with arguments: {}
2018-xx-xx xx:xx:xx,xx INFO      DRY-RUN: open: logstash-2018.04.05 with arguments: {}
2018-xx-xx xx:xx:xx,xx INFO      Action ID: 1, "open" completed.
2018-xx-xx xx:xx:xx,xx INFO      Job completed.
//}

次にDRY-RUNオプションを外してCuratorを実行します。


//list[curator-26][Curatorでclose実行]{
curator ~/.curator/open_indices.yml
//}

DRY-RUNで実行した時と同様にログを確認します。

//list[curator-27][ログの確認]{
INFO      Preparing Action ID: 1, "open"
INFO      Trying Action ID: 1, "open": open logstash index
INFO      Opening selected indices: ['logstash-2018.04.05', 'logstash-2018.04.04', 'logstash-2018.04.03', 'logstash-2018.04.02', 'logstash-2018.04.01']
INFO      Action ID: 1, "open" completed.
INFO      Job completed.
//}


curlコマンドを利用して、indexが存在することを確認します。

//list[curator-28][indexの確認]{
curl -XGET localhost:9200/_cat/indices/logstash* | sort
//}

Closeしたindexが再びopenされ、利用できる状態となりました。

//cmd{
curl -XGET localhost:9200/_cat/indices/logstash* | sort
yellow open logstash-2018.04.01 5 1 8 0  93.2kb  93.2kb
yellow open logstash-2018.04.02 5 1 9 0 102.8kb 102.8kb
yellow open logstash-2018.04.03 5 1 9 0 102.8kb  72.8kb
yellow open logstash-2018.04.04 5 1 4 0  14.5kb  14.5kb
yellow open logstash-2018.04.05 5 1 4 0  14.5kb 104.5kb
//}


今回はCuratorの機能の一部分だけを紹介しました。
みなさんのユースケースに合わせて、Curatorを使いこなしていただけると幸いです。
