= Curator

Curatorは、Elasticsearchに保存したログのインデックス操作や、スナップショットの取得などを行う運用支援ツールです。
この章は、以下の操作を行います。

 * インデックスの削除
 * インデックスのCloseとOpen


Curatorの実行環境を構築します。


=== Curatorのインストール

Curatorのパッケージを取得するため、リポジトリの登録をします。

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


== インデックスの削除

ログ分析などの運用を行うと、大量のログデータが溜まっていきます。その結果、ディスクを圧迫したり、パフォーマンス低下などが発生する可能性があります。
そこでCuratorの登場です。
Curatorを使用することで、任意の期間でインデックスを削除することが可能です。


補足ですが、@<chapref>{logstash}や@<chapref>{beats}でElasticsearchに保存したログは、日ごとにインデックスが作成されていきます。
例えば、2018年4月1日にLogstashがElasticsearchに保存したログは、@<code>{logstash-2018.04.01}というインデックスに保存されます。
2018年4月2日に保存したログは、@<code>{logstash-2018.04.02}というインデックスに保存されます。
100日経過した時には、100インデックスが作成されます。
このように時系列で作成していくインデックスを時系列インデックスと言います。


=== インデックスの削除操作

インデックスが2018年4月1日〜4月5日まであるとします。
Curlでインデックスを確認します。

//list[curator-03][インデックスの確認]{
curl -XGET localhost:9200/_cat/indices/logstash* | sort
yellow open logstash-2018.04.01 5 1 8 0  93.2kb  93.2kb
yellow open logstash-2018.04.02 5 1 9 0 102.8kb 102.8kb
yellow open logstash-2018.04.03 5 1 9 0 102.8kb  72.8kb
yellow open logstash-2018.04.04 5 1 4 0  14.5kb  14.5kb
yellow open logstash-2018.04.05 5 1 4 0  14.5kb 104.5kb
//}

Curatorは、設定ファイルとアクションファイルの二つのファイルで構成されています。


設定ファイルの@<code>{curator.yml}を作成します。

//list[curator-04][curator.ymlの作成]{
vim ~/.curator/curator.yml
//}

//cmd{
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

//table[curator-05][curator.ymlの設定項目]{
No.	Item	Content
-----------------
1	hosts	ElasticsearchのIPアドレスを指定
2	port	Elasticsearchのポートを指定
3	logfile	ログファイルの出力先の指定
//}

次にインデックス削除を定義したアクションファイルの@<code>{delete_indices.yml}を作成します。
今回は、1日分のインデックス保持させるためunit_countを1を指定します。

//list[curator-06][delete_indices.ymlの作成]{
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

//table[curator-07][delete_indices.ymlの設定項目]{
No.	Item	Content
-----------------
1	action	アクションを指定します（今回は、削除を指定）
2	filtertype	インデックスのフィルタ方式を指定します
3	pattern	任意のインデックスパターンを指定する場合に使用します
4	age	時間指定する場合に使用します
5	unit	時間単位を指定します
6	unit_count	時間を指定します（今回は、unitでdayを指定しているため、1日分を保持）
//}


インデックス削除の環境が整ったので実行します。
Curatorのコマンドラインで実行する際の引数は以下です。
オプションの--configは、@<code>{curator.yml}を@<code>{~/.curator/curator.yml}以外に配置した場合に使用します。
今回は、@<code>{~/.curator/curator.yml}に配置しているため、--configオプションは使用しません

//list[curator-08][Curatorの実行引数]{
curator [--config CONFIG.YML] [--dry-run] delete_indices.yml
//}

それではDRY-RUNで実行します。
DRY-RUNを使用することで、設定ファイルに不備がないかを確認することができます。


//list[curator-09][CuratorをDRY-RUNで削除実行]{
curator --dry-run ~/.curator/delete_indices.yml
//}

ログで実行結果を確認します。
DRY-RUNで実行した場合は、ログにDRY-RUNと表記されます。
最新のインデックス以外は削除対象という結果がログからわかります。

//list[curator-10][ログの確認]{
cat /var/log/curator
//}

//cmd{
2018-xx-xx xx:xx:xx,xxx INFO      Preparing Action ID: 1, "delete_indices"
2018-xx-xx xx:xx:xx,xxx INFO      Trying Action ID: 1, "delete_indices": delete logstash index
2018-xx-xx xx:xx:xx,xxx INFO      DRY-RUN MODE.  No changes will be made.
2018-xx-xx xx:xx:xx,xxx INFO      (CLOSED) indices may be shown that may not be acted on by action "delete_indices".
2018-xx-xx xx:xx:xx,xxx INFO      DRY-RUN: delete_indices: logstash-2018.04.01 with arguments: {}
2018-xx-xx xx:xx:xx,xxx INFO      DRY-RUN: delete_indices: logstash-2018.04.02 with arguments: {}
2018-xx-xx xx:xx:xx,xxx INFO      DRY-RUN: delete_indices: logstash-2018.04.03 with arguments: {}
2018-xx-xx xx:xx:xx,xxx INFO      DRY-RUN: delete_indices: logstash-2018.04.04 with arguments: {}
2018-xx-xx xx:xx:xx,xxx INFO      Action ID: 1, "delete_indices" completed.
2018-xx-xx xx:xx:xx,xxx INFO      Job completed.
//}


次にDRY-RUNを外して実行します。

//list[curator-11][Curatorで削除実行]{
curator ~/.curator/delete_indices.yml
//}

DRY-RUNで実行した時と同様にログを確認します。

//list[curator-12][ログの確認]{
2018-xx-xx xx:xx:xx,xxx INFO      Preparing Action ID: 1, "delete_indices"
2018-xx-xx xx:xx:xx,xxx INFO      Trying Action ID: 1, "delete_indices": delete logstash index
2018-xx-xx xx:xx:xx,xxx INFO      Deleting selected indices: ['logstash-2018.04.10', 'logstash-2018.04.11']
2018-xx-xx xx:xx:xx,xxx INFO      DRY-RUN: delete_indices: logstash-2018.04.01 with arguments: {}
2018-xx-xx xx:xx:xx,xxx INFO      DRY-RUN: delete_indices: logstash-2018.04.02 with arguments: {}
2018-xx-xx xx:xx:xx,xxx INFO      DRY-RUN: delete_indices: logstash-2018.04.03 with arguments: {}
2018-xx-xx xx:xx:xx,xxx INFO      DRY-RUN: delete_indices: logstash-2018.04.04 with arguments: {}
2018-xx-xx xx:xx:xx,xxx INFO      Action ID: 1, "delete_indices" completed.
2018-xx-xx xx:xx:xx,xxx INFO      Job completed.
//}

最後に、curlでインデックスが削除されているか確認します。
2018年4月5日のインデックスのみが保存されていることがわかります。

//list[curator-13][インデックスの確認]{
curl -XGET localhost:9200/_cat/indices/logstash* | sort
yellow open logstash-2018.04.05 5 1 4 0  14.5kb 104.5kb
//}


== インデックスのCloseとOpen

インデックスの削除を先ほど行いました。
次は、インデックスのCloseです。
ログをElasticsearchに保存し続けたいが、パフォーマンスは低下させたくないといった時にCloseを利用します。
インデックスをCloseすることでメモリを解放します。
Closeしたインデックスは、再度利用する場合にopenを使用します。

=== インデックスのClose


インデックス削除を実施した時と同様に2018年4月1日〜4月5日までのインデックスがあるとします。
Curlでインデックスを確認します。

//list[curator-14][インデックスの確認]{
curl -XGET localhost:9200/_cat/indices/logstash* | sort
yellow open logstash-2018.04.01 5 1 8 0  93.2kb  93.2kb
yellow open logstash-2018.04.02 5 1 9 0 102.8kb 102.8kb
yellow open logstash-2018.04.03 5 1 9 0 102.8kb  72.8kb
yellow open logstash-2018.04.04 5 1 4 0  14.5kb  14.5kb
yellow open logstash-2018.04.05 5 1 4 0  14.5kb 104.5kb
//}


すでに@<code>{curator.yml}は、作成してあるので、@<code>{close_indices.yml}を作成します。
対象は、最新のインデックス以外をclose設定とします。
また、actionは、closeを指定します。

//list[curator-15][close_indices.ymlの作成]{
vim ~/.curator/close_indices.yml
//}

//cmd{
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


それではDRY-RUNで実行します。

//list[curator-16][CuratorをDRY-RUNでClose実行]{
curator --dry-run ~/.curator/close_indices.yml
//}

DRY-RUNでログの実行結果を確認します。
close対象のインデックスがログの結果からわかります。


//list[curator-17][ログの確認]{
cat /var/log/curator
//}

//cmd{
2018-xx-xx xx:xx:xx,xxx INFO      Preparing Action ID: 1, "close"
2018-xx-xx xx:xx:xx,xxx INFO      Trying Action ID: 1, "close": close logstash index
2018-xx-xx xx:xx:xx,xxx INFO      DRY-RUN MODE.  No changes will be made.
2018-xx-xx xx:xx:xx,xxx INFO      (CLOSED) indices may be shown that may not be acted on by action "close".
2018-xx-xx xx:xx:xx,xxx INFO      DRY-RUN: close: logstash-2018.04.01 with arguments: {'delete_aliases': False}
2018-xx-xx xx:xx:xx,xxx INFO      DRY-RUN: close: logstash-2018.04.02 with arguments: {'delete_aliases': False}
2018-xx-xx xx:xx:xx,xxx INFO      DRY-RUN: close: logstash-2018.04.03 with arguments: {'delete_aliases': False}
2018-xx-xx xx:xx:xx,xxx INFO      DRY-RUN: close: logstash-2018.04.04 with arguments: {'delete_aliases': False}
2018-xx-xx xx:xx:xx,xxx INFO      Action ID: 1, "close" completed.
2018-xx-xx xx:xx:xx,xxx INFO      Job completed.
//}

次にDRY-RUNを外して実行します。


//list[curator-18][Curatorでclose実行]{
curator ~/.curator/close_indices.yml
//}

DRY-RUNで実行した時と同様にログを確認します。


//list[curator-19][ログの確認]{
2018-xx-xx xx:xx:xx,xxx INFO      Preparing Action ID: 1, "close"
2018-xx-xx xx:xx:xx,xxx INFO      Trying Action ID: 1, "close": close logstash index
2018-xx-xx xx:xx:xx,xxx INFO      Closing selected indices: ['logstash-2018.04.01', 'logstash-2018.04.02', 'logstash-2018.04.03', 'logstash-2018.04.04']
2018-xx-xx xx:xx:xx,xxx INFO      Action ID: 1, "close" completed.
2018-xx-xx xx:xx:xx,xxx INFO      Job completed.
//}

最後に、curlでインデックスがcloseされているか確認します。
2018年4月5日以外のインデックスがcloseに変更されていることがわかります。

//list[curator-20][インデックスの確認]{
curl -XGET localhost:9200/_cat/indices/logstash* | sort
yellow close logstash-2018.04.01 5 1 8 0  93.2kb  93.2kb
yellow close logstash-2018.04.02 5 1 9 0 102.8kb 102.8kb
yellow close logstash-2018.04.03 5 1 9 0 102.8kb  72.8kb
yellow close logstash-2018.04.04 5 1 4 0  14.5kb  14.5kb
yellow open logstash-2018.04.05 5 1 4 0  14.5kb 104.5kb
//}


=== インデックスのOpen

先ほどCloseしたインデックスを対象にopenを行います。


@<code>{open_indices.yml}を作成します。
過去5日分を対象にOpen設定とするため、unit_countを5に設定します。
directionで新しい方から数えるのか、古い方から数えるのかを指定できます。
今回は、新しい方から数えるため、youngerを指定します（古い方からの場合は、olderです）

//list[curator-21][open_indices.ymlの作成]{
vim ~/.curator/open_indices.yml
//}

//cmd{
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

DRY-RUNで実行します。


//list[curator-22][CuratorをDRY-RUNでOpen実行]{
curator --dry-run ~/.curator/open_indices.yml
//}

DRY-RUNでログで実行結果を確認します。
open対象のインデックスがログの結果からわかります。


//list[curator-23][ログの確認]{
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

次にDRY-RUNを外して実行します。


//list[curator-24][Curatorでclose実行]{
curator ~/.curator/open_indices.yml
//}

DRY-RUNで実行した時と同様にログを確認します。

//list[curator-25][ログの確認]{
2018-xx-xx xx:xx:xx,xxx INFO      Preparing Action ID: 1, "open"
2018-xx-xx xx:xx:xx,xxx INFO      Trying Action ID: 1, "open": open logstash index
2018-xx-xx xx:xx:xx,xxx INFO      Opening selected indices: ['logstash-2018.04.05', 'logstash-2018.04.04', 'logstash-2018.04.03', 'logstash-2018.04.02', 'logstash-2018.04.01']
2018-xx-xx xx:xx:xx,xxx INFO      Action ID: 1, "open" completed.
2018-xx-xx xx:xx:xx,xxx INFO      Job completed.
//}


最後に、curlでインデックスがopenされているか確認します。
インデックスがopenされていることがわかります。

//list[curator-26][インデックスの確認]{
curl -XGET localhost:9200/_cat/indices/logstash* | sort
yellow open logstash-2018.04.01 5 1 8 0  93.2kb  93.2kb
yellow open logstash-2018.04.02 5 1 9 0 102.8kb 102.8kb
yellow open logstash-2018.04.03 5 1 9 0 102.8kb  72.8kb
yellow open logstash-2018.04.04 5 1 4 0  14.5kb  14.5kb
yellow open logstash-2018.04.05 5 1 4 0  14.5kb 104.5kb
//}


Curatorは、ここでは紹介できていない便利な機能がまだまだあります。
ぜひ色々と試して運用を効率的にしていってもらえればと思います。
