
= Beats


Beatsは、シンプルなデータ取り込みツールです。
あれ？Logstashは？と思う方もいると思いますが、Logstashは、豊富な機能を持ってます。
改めてLogstashの役割を説明すると、様々なデータソースからデータを取得し、意味のあるフィールドに変換し、指定のOutput先にストリーミング処理をします。
このことからETLとしてLogstashが必要な機能を持っているということがわかるかと思います。
ではなぜ、LogstashだけでなくBeatsが登場したかというと、パフォーマンス問題を抱えていたためです。
Logstashは、複数のパイプラインや高度なフィルタリングを施すことができますが、その分メモリを多く消費します。
そこで軽量で手軽に導入できるBeatsが登場しました。
何が手軽かというと、設定ファイルがYAMLで全て完結するのです。
しかも、設定箇所も複数存在するわけではなく、最低限の設定で十分な機能を提供します。


== Beats Family

Beatsはどんな種類があるのかを改めて記載します。

 * Filebeat
 * Metricbeat
 * Packetbeat
 * Winlogbeat
 * Auditbeat
 * Heartbeat

今回は、3つのBeatsの利用方法について触れていきます。

 * Filebeat
 * Metricbeat
 * Auditbeat


== Filebeat


Filebeatは、ログを一箇所に転送する用途で使用します。
また、TLS暗号化をサポートしているため、セキュアに転送することができます。
たとえば、以下の構成図がFilebeatのよくある構成です。

//image[filebeat01][Filebeatの構成]{
//}

Filebeatをデータソースであるサーバに導入し、Logstashへ転送する構成です。
Logstashに転送することでログを集約することができます。
また、Filebeatから転送されたデータを分析しやすい構造に変換する処理を行い、Elasticsearchに保存します。


この他にもFilebeatは、Moduleを利用することで一部のデータを分析しやすいフィールド構造に変換することもできます。
Moduleについては、後ほど説明します。


それでは、Filebeatでデータを取得し、Logstashに転送し、Elasticsaerchに保存するところまでをみていきたいと思います。


=== Filebeatの構成について

Filebeatを試す環境は、@<chapref>{logstash}を元として構成します。
新たにFilebeatとNginxを追加します。


今回想定するケースは、NginxのアクセスログををFilebeatが取得し、Logstashに転送します。
Logstashは、Filebeatから転送されたログをElasticsearchに保存するところまでを行います。


//image[filebeat02][サーバの構成について]{
//}

それでは、FilebeatとNginxのインストールを実施していきます。


=== Filebeatをインストール


Filebeatをインストールします。@<chapref>{logstash}でyumリポジトリの登録が完了していることを前提として進めます。


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

ここからはFilebeatがNginxのアクセスログを取得し、Logstashに転送し、LogstashがElasticsearchに取り込む設定を行なっていきます。


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
NGINX_ACCESS_LOG %{IPORHOST:client_ip} (?:-|(%{WORD}.%{WORD})) %{USER:ident} \[%{HTTPDATE:date}\] "(?:%{WORD:verb} %{NOTSPACE:request}(?: HTTP/%{NUMBER:ver})?|%{DATA:rawrequest})" %{NUMBER:response} (?:%{NUMBER:bytes}|-) %{QS:referrer} %{QS:agent} %{QS:forwarder}
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


パターンファイルのInputにFilebeatからデータを受信するためBeatsブラグインを使用します。
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

Outputは、@<chapref>{logstash}の設定と同様です。


最後に、作成したパイプラインファイルを読み込むため、pipelines.ymlの設定をします。
@<code>{logstash_pipelines}の設定が残っている場合、削除してください。

//list[beats-10][logstash_pipelinesの編集]{
- pipeline.id: filebeat
  pipeline.batch.size: 125
  path.config: "/etc/logstash/conf.d/filebeat.cfg"
  pipeline.workers: 1
//}  

これでFilebeatとLogstashの環境が整いました。
Filebeatから起動してしまうと転送先のLogstashにBeats設定が反映されていないため、エラーになります。
そのため、Logstashから起動します。

//list[beats-11][Logstashの起動]{
sudo initctl start logstash
logstash start/running, process 3845
//}

Filebeatを起動します。
"config OK"と標準出力されれば問題なく起動しています。

//list[beats-12][Filebeatの起動]{
sudo initctl start logstash
logstash start/running, process 3845
//}


=== 動作確認

Elasticsearchにインデクシングされているかを確認します。
想定通り取り込まれていることがわかります。

Elasticsearchにデータが転送されているか、curlを発行して確認します。
以下のように@<code>{logstash-YYYY.MM.dd}で出力されていれば正常に保存されています。

//list[beats-13][Filebeatの起動]{
curl -XGET localhost:9200/_cat/indices/logstash*
yellow open logstash-2018.04.10 fzIOfXzOQK-p0_mmvO7wrw 5 1 8 0 93.2kb 93.2kb
//}

補足ですが、ステータスが"yellow"になっているのは、ノードが冗長化されていないため表示されています。
今回は、1ノード構成のため"yellow"になってしまうので、無視してください。


=== Filbeat Modules

ここまででFilebeatの使い方がわかったと思います。
しかし、これではどこが手軽なの？むしろ重厚感が増したのでは？と思われる方もいると思います。
ここからは、もっと手軽に導入するためのFilbeat Moduleについて触れていきたいと思います。


Filebeat Modulesは、あらかじめデータソースに対応した Moduleが用意されています。
このModuleを使用することで、Logstashで複雑なフィルタなどを書かずに、意味のあるフィールドに変換し、Elasticsearchにインデクシングできます。
また、Kibanaのダッシュボードも用意されているため、インデクシングされたデータを即時でビジュアライズすることができます。


==== Filebeat Modulesの構成

Filebeatのデータソースは、Nginxのアクセスログとします。
先ほどの構成は、Logstashに転送していましたが、Elasticsaerchに直接保存する構成とします。

//image[filebeat03][Filebeatの構成]{
//}


==== Ingest Node Pluginをインストール

Filebeat Modulesは、パイプラインを自動で作成します。
その際にUserAgent、GeoIPの解析をするため、@<code>{Ingest Node Plugin}と@<code>{Ingest GeoIP plugin}をインストールします。


//list[beats-14][Ingest Node Pluginのインストール]{
/usr/share/elasticsearch/bin/elasticsearch-plugin install ingest-user-agent
//}

//list[beats-15][Ingest GeoIP pluginのインストール]{
/usr/share/elasticsearch/bin/elasticsearch-plugin install ingest-geoip
//}


インストールが完了したらElasticsearchを再起動します。


//list[beats-16][Elasticsaerchの再起動]{
sudo service elasticsearch restart
//}

今回は@<code>{Nginx Module}を例にModuleを利用すると、どのくらい構築コストが減少するのかを検証します。
@<chapref>{logstash}でKibanaをインストールしている環境を引き続き利用することを前提として
話を進めますが、もし新しい環境で始める場合は@<chapref>{logstash}や@<chapref>{Kibana-visualize}を参考に
環境構築を行ってください。


==== Filebeat Modulesの設定

Filebeatの設定ファイルを編集しますので、以下の設定ファイル@<code>{filebeat.yml}を使用します。
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

@<code>{filebeat.yml}の編集内容について説明します。
Nginxの有効化を行います。Nginxのアクセスログのパス設定ですが、インストールした状態（デフォルト）のまま利用するのであればパスの変更は不要です。
今回はデフォルト設定のまま利用しています。


//list[beats-17][filebeat.ymlのNginx Module編集]{
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


今回の設定では、アウトプット先を複数にする設定は発生しないと思いますが、
もし既存の設定が残っていた場合は、再起動時に以下のエラーが発生します。
このエラーが発生した場合は、アウトプット先が複数の可能性があるので確認してください。


//list[beats-21][複数アウトプット指定した際のエラー]{
error unpacking config data: more than one namespace configured accessing 'output' (source:'/etc/filebeat/filebeat.yml')
//}

では、いよいよFilebeatを起動します。


//list[beats-23][Filebeatの起動]{
sudo service filebeat start
//}


あとは、データが取り込まれているかをKibana@<href>{http://{Global_IP\}:5601}を開いて確認します。


トップページが開きます。
左ペインにあるManagementをクリックします。


//image[filebeat04][Managementをクリック]{
//}

Index Patternsをクリックします。

//image[filebeat05][Indexを選択]{
//}

Filebeatのインデックスパターンが登録されていることがわかります。

//image[filebeat06][Filebeatのインデックスパターンを確認]{
//}

左ペインにある@<code>{Dashboard}をクリックします。
様々なDashboardが登録されていることがわかります。
Logstashなどでログを取り込んだ場合は、Dashboardを一から作成する必要がありますが、Beatsの場合は、あらかじめ用意されています。

//image[filebeat07][Dashboardの確認]{
//}

今回は、Nginxの@<code>{Filebeat Nginx Overview}というDashboardをクリックします。
取り込んだログがDashboardに表示されていることがわかります。

//image[filebeat08][Filebeat Nginx Overview]{
//}

いかがでしたか？
他にも取り込みたいログがあれば、@<code>{filebeat.yml}のModuleを追加するだけで容易にモニタリングができるようになります。
追加する場合は、@<code>{filebeat.reference.yml}にModulesが記載されているので、コピー&ペーストして有効化してください。


次は、サーバのリソースを容易にモニタリングすることを可能とするMetricbeatについてです。


== Metricbeat

Metricbeatは、サーバのリソース(CPU/Mem/process..など)を容易にモニタリングすることができます。
その他にもDockerやElasticsaerchなどにも対応しており、様々なプロダクトをモニタリングすることが可能です。

また、先ほどのFilebeatと同様にYAMLを編集するだけなので、学習コストもかかりません。
今回は、サーバのメトリックをモニタリングできるところまで見たいと思います。それでは、早速インストールしていきます。


//list[beats-24][Metricbeatのインストール]{
sudo yum install metricbeat
//}


MetricbeatもFilebeat同様にベースの設定ファイル@<code>{metricbeat.reference.yml}があるのですが、
デフォルト有効化されているModuleが多いため、以下の設定ファイルを使用します。
既存で設定してある内容は全て上書きしてください。


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

設定が完了したのでMetricbeatを起動します。


//list[beats-26][Metricbeatの起動]{
sudo service metricbeat start
//}


Filebeatと同様にデータが取り込まれているかをKibanaを開いて確認します。
ブラウザを開いてKibana（@<href>{http://{Global_IP\}:5601}）へアクセスします。

Index Patternsの画面を開くとFilebeatのインデックスパターンの他にMetricbeatのインデックスパターンがあることがわかります。

//image[metricbeat01][Metricbeatのインデックスを確認その1]{
//}

Dashboardをクリックし、Metricbeatのインデックスを確認します。

//image[metricbeat02][Metricbeatのインデックスを確認その2]{
//}

今回は、@<code>{Metricbeat System Host Overview}というDashboardをクリックします。
CPUやメモリ、プロセスの状態をニアリアルタイムにモニタリングができていることがわかります。

//image[metricbeat03][MetricbeatのDashboard]{
//}

このようにサーバやコンテナなどにMetricbeatを導入することで一元的にサーバの状態をモニタリングすることができます。
最後に、監査ログを容易に取り込むためのAuditbeatについてみていきます。


== Auditbeat

サーバの監査としてauditdが出力する@<code>{audit.log}をモニタリングしている方は多くいるのではないでしょうか。
@<code>{audit.log}を保管するだけでなく、ニアリアルタイムにモニタリングするためにLogstashなどのツールを利用している方もいると思います。
ただ、これから@<code>{audit.log}をモニタリングしたいという人からしたらハードルが高く、モニタリングするまでに時間を要してしまいます。
そこで、Beatsは、Auditbeatというデータシッパーがあるので容易に導入することができます。
ここまでFilbeatやMetricbeatを触ってきたらわかるとおり、学習コストはほぼかからないでDashboardで閲覧するところまでできてしまいます。

それでは、ここからAuditbeatをインストールします。


//list[beats-27][Auditbeatのインストール]{
sudo yum install auditbeat
//}

@<list>{beats-28}の@<code>{auditbeat.yml}を既存で設定してある内容は全て上書きしてください。


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


設定が完了したので、Auditbeatを起動します。


//list[beats-29][Auditbeatの起動]{
sudo service auditbeat start
//}


データが取り込まれているかをKibanaを開いて確認します。
ブラウザを開いてKibanaへアクセスします。

@<code>{Index Patterns}の画面を開くとFilebeatのインデックスパターンの他にAuditbeatのインデックスパターンがあることがわかります。

//image[auditbeat01][Auditbeatのインデックス確認]{
//}

左ペインにあるDashboardをクリックします。
検索ウィンドウから@<code>{Auditbeat}と入力すると様々なDashboardがヒットします。

//image[auditbeat02][AuditbeatのDashboardを確認]{
//}

@<code>{Auditbeat File Integrity Overview}や@<code>{Auditbeat Auditd Overview}からモニタリングが可能です。


//image[auditbeat03][Auditbeatを用いたモニタリング]{
//}

Beatsの機能、いかがだったでしょうか？
Moduleを有効化するだけで、簡単にサーバの情報を可視化できる環境が手に入ります。
他のBeatsについては今回扱いませんが、少ない学習コストで情報の可視化が可能です。みなさんもぜひ試してみてはいかがでしょうか。


== まとめ


いかがでしたか？
LogstashとBeatsの両方を操作できると、ログ収集時の選択肢が増えます。
2つのプロダクトの違いを理解できると、状況に合わせて適切なプロダクトの使い分けが可能となります。
みなさんがログと素敵な時間を過ごせることを願っています。

@micci184
