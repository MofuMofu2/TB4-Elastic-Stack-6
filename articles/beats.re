
= Beatsを体験する


Beatsはデータを取得することに重きを置いたツールです。
Logstashは複数のパイプラインや高度なフィルタリングを行うことが可能ですが、その分メモリを多く消費します。
そこで、軽量で手軽に導入できるBeatsが登場しました。

Beatsの設定ファイルはYAMLで全て完結します。よって、手軽に設定・動作させることが可能なのです。


== Beats Family

Beatsにはどんな種類があるのかを紹介します。

 * Filebeat
 * Metricbeat
 * Packetbeat
 * Winlogbeat
 * Auditbeat
 * Heartbeat

今回はこの中から、次の3つのBeatsの利用方法について触れていきます。

 * Filebeat
 * Metricbeat
 * Auditbeat


== Filebeat


Filebeatは、ログを一箇所に転送する用途で使用します。
また、TLS暗号化をサポートしているため、セキュアに転送することができます。
たとえば、@<img>{filebeat01}の構成図がFilebeatのよくある構成です。

//image[filebeat01][Filebeatの構成]{
//}

Filebeatをデータソースであるサーバに導入し、Logstashへ転送する構成です。
Logstashに転送することでログを集約することができます。
また、Filebeatから転送されたデータを分析しやすい構造に変換する処理を行い、Elasticsearchに保存します。


この他にもFilebeatは、Moduleを利用することで一部のデータを分析しやすいフィールド構造に変換することもできます。
Moduleについては、後ほど説明します。


それではFilebeatでデータを取得し、Elasticsaerchに保存するところまでの一連の流れをみていきます。


=== Filebeatの構成について

Filebeatを試す環境は、「AWSでLogstashを使ってみる」を元に構成します。
新たにFilebeatとNginxを追加します。


今回想定するケースは、NginxのアクセスログをFilebeatが取得し、Logstashに転送するというものです。
Logstashは、Filebeatから転送されたログをElasticsearchに保存するところまでを行います。


//image[filebeat02][サーバの構成について][scale=0.8]{
//}

それでは、FilebeatとNginxをインストールしていきます。


=== Filebeatをインストール


Filebeatをインストールします。「AWSでLogstashを使ってみる」でyumリポジトリの登録が完了していることを前提として進めます。

//list[beats-01][Filebeatsのインストール]{
sudo yum install filebeat
//}


=== Nginxの環境を構築する

Nginxをインストールします。

//list[beats-02][Nginxのインストール]{
sudo yum install nginx
//}

インストールが完了したら、Nginxを起動します。

//list[beats-03][Nginxの起動]{
sudo service nginx start
//}


Nginxに対してcurlを実行し、アクセスログが出力されているかを確認します。
また、ステータスコード200が返ってきていることも合わせて確認します。

//list[beats-04][Nginxの動作確認]{
curl localhost
tail -f /var/log/nginx/access.log
127.0.0.1 - - [xx/xxx/2018:xx:xx:xx +0000] "GET / HTTP/1.1" 200 3770 "-" "curl/7.53.1" "-"
//}


これでFilebeatとNginxの環境が構築できました。

=== FilebeatからLogstashへ転送

次はFilebeatでNginxのアクセスログを取得し、Logstashへデータ転送をする設定を行います。


==== FilebeatとLogstashの設定

filebeat.ymlを編集します。
filebeat.prospectorsを有効化し、Nginxのアクセスログのパスを指定します。
output.logstashで転送先のLogstashを指定します。
今回は、ローカルホストですが、ネットワーク越しの場合は、IPアドレスやホスト名を指定してください。


設定を反映させるにはFilebeatの再起動が必要ですが、Logstashの設定を実施後に行います。

//list[beats-05][filebeat.ymlの編集]{
######################## Filebeat Configuration ############################

#=========================== Filebeat prospectors =============================
filebeat.prospectors:

#------------------------------ Log prospector --------------------------------
- type: log
  enabled: true
  paths:
    - /var/log/nginx/access.log

#================================ Outputs ======================================

#----------------------------- Logstash output ---------------------------------
output.logstash:
  hosts: ["localhost:5044"]

#================================ Logging ======================================
#logging.level: info
//}


次にFilebeatの転送先であるLogstashの設定を行います。
新しくパイプラインファイルとパターンファイルを作成します。

Filebeatで取得したNginxのアクセスログは、フィールド分割されていないため分析できない構造です。
そのため、分析しやすい構造にするためパターンファイルを作成します。

//list[beats-06][パターンファイルの作成]{
vim /etc/logstash/patterns/nginx_patterns
NGINX_ACCESS_LOG %{IPORHOST:client_ip} (?:-|(%{WORD}.%{WORD})) （紙面の都合により改行）
%{USER:ident} \[%{HTTPDATE:date}\] "(?:%{WORD:verb} （紙面の都合により改行）
%{NOTSPACE:request}(?: HTTP/%{NUMBER:ver})?|%{DATA:rawrequest})"（紙面の都合により改行）
%{NUMBER:response} (?:%{NUMBER:bytes}|-) %{QS:referrer} %{QS:agent} %{QS:forwarder}
//}

パイプラインファイルを作成します。

//list[beats-07][パイプラインファイルの作成]{
input {
  beats {
    port => "5044"
  }
}
filter {
  grok {
    patterns_dir => ["/etc/logstash/patterns/nginx_patterns"]
    match => { "message" => "%{NGINX_ACCESS_LOG}" }
  }
  date {
    match => ["date", "dd/MMM/YYYY:HH:mm:ss Z"]
    timezone => "Asia/Tokyo"
    target => "@timestamp"
  }
  geoip {
    source => "client_ip"
  }
}
output {
  elasticsearch {
    hosts => [ "localhost:9200" ]
  }
}
//}


パターンファイルのInputにFilebeatからデータを受信するため、Beatsプラグインを使用します。
Beatsプラグインは、デフォルトでインストールされています。
ポートは、先ほどfilebeat.ymlのoutput.logstashで指定したポートを指定します。
今回は、デフォルトの5044とします。

//list[beats-08][パイプラインファイルのInputについて]{
input {
  beats {
    port => "5044"
  }
}
//}

パターンファイルを指定します。
dateオプションでNginxの日付パターンを指定します。

//list[beats-09][パイプラインファイルのOutputについて]{
filter {
  grok {
    patterns_dir => ["/etc/logstash/patterns/nginx_patterns"]
    match => { "message" => "%{NGINX_ACCESS_LOG}" }
  }
  date {
    match => ["date", "dd/MMM/YYYY:HH:mm:ss Z"]
    timezone => "Asia/Tokyo"
    target => "@timestamp"
  }
  geoip {
    source => "client_ip"
  }
}
//}

Outputは、「AWSでLogstashを使ってみる」の設定と同様です。


最後に、作成したパイプラインファイルを読み込むため、pipelines.ymlの設定をします。
@<code>{logstash_pipelines}の設定が残っている場合、削除してください。

//list[beats-10][logstash_pipelinesの編集]{
- pipeline.id: filebeat
  pipeline.batch.size: 125
  path.config: "/etc/logstash/conf.d/filebeat.cfg"
  pipeline.workers: 1
//}

これでFilebeatとLogstashの環境が整いました。
Filebeatから起動すると、LogstashがFilebeatからデータを受け付ける設定が反映されていないため、エラーになってしまいます。
そのため、Logstashから起動します。

//list[beats-11][Logstashの起動]{
sudo initctl start logstash
//}

Filebeatを起動します。
"config OK"と標準出力されれば問題なく起動しています。

//list[beats-12][Filebeatの起動]{
sudo initctl start logstash
//}


=== 動作確認


Elasticsearchにデータが転送されているか、curlコマンドを利用して確認します。
@<list>{beats-13}のように@<code>{logstash-YYYY.MM.dd}で出力されていれば正常に保存されています。

//list[beats-13][Filebeatの起動]{
curl -XGET localhost:9200/_cat/indices/logstash*
//}

//cmd{
curl -XGET localhost:9200/_cat/indices/logstash*
yellow open logstash-2018.04.10 fzIOfXzOQK-p0_mmvO7wrw 5 1 8 0 93.2kb 93.2kb
//}


補足ですが、ステータスが"yellow"になっているのは、ノードが冗長化されていないため表示されています。
今回は、1ノード構成のため"yellow"になってしまうので、無視してください。


=== Filbeat Modules

Filebeatの利用方法をひととおり紹介してきました。
これではどこが手軽なの？むしろ重厚感が増したのでは？と思われる方がいるかもしれません。

ここからは、Beatsをさらに手軽に導入できる、@<code>{Filbeat Module}について触れていきたいと思います。


Filebeat Modulesでは、あらかじめデータソースに対応したModuleが用意されています。
このModuleを使用することで、Logstashで複雑なフィルタなどを書くことなく、データの収集・加工・Elasticsearchへの保存が可能です。
また、Elasticsearchへ保存されたデータをKibanaを用いて可視化することも可能です。この際、Kibanaのグラフを作成する必要はありません。

==== Filebeat Modulesの構成

Filebeatのデータソースは、Nginxのアクセスログを利用します。
Nginxのアクセスログは、Logstashを介さずにElasticsaerchに直接転送します。


==== Ingest Node Pluginをインストール

Filebeat Modulesは、パイプラインを自動で作成します。
その際にUserAgent、GeoIPの解析をするため、@<code>{Ingest Node Plugin}と@<code>{Ingest GeoIP plugin}をインストールします。


//list[beats-14][Ingest Node Pluginのインストール]{
/usr/share/elasticsearch/bin/elasticsearch-plugin install ingest-user-agent
//}

//list[beats-15][Ingest GeoIP pluginのインストール]{
/usr/share/elasticsearch/bin/elasticsearch-plugin install ingest-geoip
//}

プラグインのインストールが完了した後、Elasticsearchを再起動します。


//list[beats-16][Elasticsaerchの再起動]{
sudo service elasticsearch restart
//}

「AWSでLogstashを使ってみる」でKibanaをインストールしている環境を引き続き利用することを前提として話を進めます。
もし新しい環境でFilebeat Modulesを検証する場合、「AWSでLogstashを使ってみる」や後述の「Kibanaを使ってデータを可視化する」を参考に環境構築を行ってください。


==== Filebeat Modulesの設定

Filebeatの設定ファイルを編集するため、@<list>{beats-17}の@<code>{filebeat.yml}を使用します。
既存で設定してある内容は全て上書きしてください。

//list[beats-17][filebeat.ymlのNginx Module編集]{
######################## Filebeat Configuration ############################

#==========================  Modules configuration ============================
filebeat.modules:

#-------------------------------- Nginx Module -------------------------------
- module: nginx
  access:
    enabled: true
  error:
    enabled: true

#================================ Outputs ======================================

#-------------------------- Elasticsearch output -------------------------------
output.elasticsearch:
  enabled: true

  hosts: ["localhost:9200"]

#============================== Dashboards =====================================
setup.dashboards.enabled: true

#============================== Kibana =====================================
setup.kibana:
  #host: "localhost:5601"

#================================ Logging ======================================
#logging.level: info
//}

@<list>{beats-17}の編集内容について説明します。

まずNginxの有効化を行います。Nginxのアクセスログのパス設定ですが、インストールした状態（デフォルト）のまま利用するのであればパスの変更は不要です。
今回はデフォルト設定のまま利用しています。


//list[beats-171][filebeat.ymlのNginx Module編集]{
#-------------------------------- Nginx Module -------------------------------
- module: nginx
  access:
    enabled: true
//}


Output先をElasticsearchに変更しています。


//list[beats-18][filebeat.ymlのElasticsearch output編集]{
#-------------------------- Elasticsearch output -------------------------------
output.elasticsearch:
  enabled: true

  hosts: ["localhost:9200"]
//}


最後にKibanaのDashboardを起動時にセットアップする設定を有効化します。


//list[beats-22][KibanaのDashboardを自動で作成する]{
#============================== Dashboards =====================================
setup.dashboards.enabled: true
//}


今回の設定では、アウトプット先を複数にする設定をしていません。
もし既存の設定が残っていた場合、Beatsの再起動時に@<list>{beats-21}のエラーが発生します。
このエラーが発生した場合は、アウトプット先が複数の可能性があるので確認してください。


//list[beats-21][複数のアウトプット先を指定した際に出力されるエラー]{
error unpacking config data: more than one namespace configured accessing
'output' (source:'/etc/filebeat/filebeat.yml')
//}

では、いよいよFilebeatを起動します。


//list[beats-23][Filebeatの起動]{
sudo service filebeat start
//}


あとは、データが取り込まれているかをKibana@<href>{http://{Global_IP\}:5601}を開いて確認します。


トップページが開きます。
左ペインにあるManagementをクリックします。


//image[filebeat04][Managementをクリック][scale=0.8]{
//}

Index Patternsをクリックします。

//image[filebeat05][Indexを選択][scale=0.8]{
//}

Filebeatのindexパターンが登録されていることがわかります。

//image[filebeat06][Filebeatのindexパターンを確認][scale=0.8]{
//}

左ペインにある@<code>{Dashboard}をクリックします。
Filebeat Modulesの機能によって、あらかじめDashboardが準備されています。

//image[filebeat07][Dashboardの確認][scale=0.8]{
//}

今回は、Nginxの@<code>{Filebeat Nginx Overview}というDashboardをクリックします。
取得したログの情報がグラフィカルに表示されていますね。

//image[filebeat08][Filebeat Nginx Overview][scale=0.8]{
//}

いかがでしたか？
他にも取り込みたいログがあれば、@<code>{filebeat.yml}のModuleを追加するだけで容易にモニタリングができるようになります。
追加する場合は、@<code>{filebeat.reference.yml}にModulesが記載されているので、コピー&ペーストして有効化してください。

== Metricbeat

Metricbeatは、サーバのリソース（CPU/Mem/process……など）を容易にモニタリングすることができます。
サーバ以外ににも、DockerやElasticsearchのリソース監視も可能です。

Filebeatと同様、YAMLを編集するだけで設定が完了します。
今回は、サーバのメトリックをモニタリングできるところをゴールとします。


//list[beats-24][Metricbeatのインストール]{
sudo yum install metricbeat
//}


Metricbeatも@<code>{metricbeat.reference.yml}があらかじめ存在します。
しかし、デフォルトで有効化されているModuleが多いため、@<list>{beats-25}の@<code>{metricbeat.yml}を利用します。
すでに設定されている内容は全て上書きしてください。


//list[beats-25][/etc/metricbeat/metricbeat.ymlの編集]{
#==========================  Modules configuration ============================
metricbeat.config.modules:
  path: ${path.config}/modules.d/*.yml
  reload.enabled: false

#------------------------------- System Module -------------------------------
metricbeat.modules:
- module: system
  metricsets:
    - cpu             # CPU usage
    - filesystem      # File system usage for each mountpoint
    - fsstat          # File system summary metrics
    - load            # CPU load averages
    - memory          # Memory usage
    - network         # Network IO
    - process         # Per process metrics
    - process_summary # Process summary
    - uptime          # System Uptime
    - core           # Per CPU core usage
    - diskio         # Disk IO
    - socket         # Sockets and connection info (linux only)
  enabled: true
  period: 10s
  processes: ['.*']

  cpu.metrics:  ["percentages"]  # The other available options are normalized_percentages and ticks.
  core.metrics: ["percentages"]  # The other available option is ticks.

#==================== Elasticsearch template setting ==========================
setup.template.settings:
  index.number_of_shards: 1
  index.codec: best_compression

#============================== Dashboards =====================================
setup.dashboards.enabled: true

#============================== Kibana =====================================
setup.kibana:
  #host: "localhost:5601"

#================================ Outputs =====================================

#-------------------------- Elasticsearch output ------------------------------
output.elasticsearch:
  hosts: ["localhost:9200"]

#================================ Logging =====================================
#logging.level: debug
//}

設定ファイルの準備ができた後、Metricbeatを起動します。

//list[beats-26][Metricbeatの起動]{
sudo service metricbeat start
//}


Elasticsearchへデータが転送できたか、Kibanaを開いて確認します。
ブラウザを開いてKibana（@<href>{http://{Global_IP\}:5601}）へアクセスします。

Index Patternsの画面を開くとFilebeatのindexパターンの他にMetricbeatのindexパターンがあることがわかります。

//image[metricbeat01][Metricbeatのindexを確認その1][scale=0.8]{
//}

Dashboardをクリックし、Metricbeatのindexを確認します。

//image[metricbeat02][Metricbeatのindexを確認その2][scale=0.8]{
//}

@<code>{Metricbeat System Host Overview}というDashboardをクリックします。
CPUやメモリ、プロセスの状態がDashboardに描画されています。

//image[metricbeat03][MetricbeatのDashboard][scale=0.8]{
//}

サーバやコンテナなどにMetricbeatを導入すると、Kibanaを利用してサーバの状態をモニタリングすることができます。
KibanaのDashboardだけで全てのサーバの状態が参照できるので、運用コストを下げることが可能です。

== Auditbeat

Linuxサーバへ攻撃がないかを確認するため、auditdが出力する@<code>{audit.log}をモニタリングしている方は多いのではないでしょうか。
しかし、@<code>{audit.log}のモニタリングはハードルが高く、監視環境の構築に時間がかかります。

BeatsのAuditbeatを利用すると、FilebeatやMetricbeatのように少ない学習コストで@<code>{audit.log}のモニタリングが可能です。
早速環境を構築していきましょう。

始めに、Auditbeatをインストールします。

//list[beats-27][Auditbeatのインストール]{
sudo yum install auditbeat
//}

Auditbeatにも、既存の設定ファイル@@<code>{auditbeat.yml}が存在します。
今回の要件に合わせて@<list>{beats-28}を準備しました。設定内容を上書きして保存します。


//list[beats-28][/etc/auditbeat/auditbeat.ymlの編集]{
#==========================  Modules configuration =============================
auditbeat.modules:

- module: auditd
  audit_rules: |

- module: file_integrity
  paths:
  - /bin
  - /usr/bin
  - /sbin
  - /usr/sbin
  - /etc

#==================== Elasticsearch template setting ==========================
setup.template.settings:
  index.number_of_shards: 3

#============================== Dashboards =====================================
setup.dashboards.enabled: true
#setup.dashboards.url:

#============================== Kibana =====================================
setup.kibana:
  #host: "localhost:5601"

#================================ Outputs =====================================

#-------------------------- Elasticsearch output ------------------------------
output.elasticsearch:
  enabled: true
  hosts: ["localhost:9200"]

#================================ Logging =====================================
#logging.level: debug
//}


@<list>{beats-28}の準備ができた後、Auditbeatを起動します。


//list[beats-29][Auditbeatの起動]{
sudo service auditbeat start
//}


Elasticsearchにデータが保存されているか確認します。
ブラウザを開いてKibanaへアクセスします。

@<code>{Index Patterns}の画面を開くと、Filebeatのindexパターンの他にAuditbeatのindexパターンがあることがわかります。

//image[auditbeat01][Auditbeatのindex確認][scale=0.8]{
//}

左ペインにあるDashboardをクリックします。
検索ウィンドウから@<code>{Auditbeat}と入力すると、複数のDashboardがヒットします。

//image[auditbeat02][AuditbeatのDashboardを確認][scale=0.8]{
//}

@<code>{Auditbeat File Integrity Overview}や@<code>{Auditbeat Auditd Overview}からモニタリングが可能です。


//image[auditbeat03][Auditbeatを用いたモニタリング][scale=0.8]{
//}

Beatsの機能、いかがだったでしょうか？
Moduleを有効化するだけで、簡単にサーバの情報を可視化できる環境が手に入ります。
他のBeatsについては今回扱いませんが、少ない学習コストで情報の可視化が可能です。みなさんもぜひ試してみてはいかがでしょうか。

