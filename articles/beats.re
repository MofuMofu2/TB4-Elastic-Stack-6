
= Beats


Beatsは、シンプルなデータ取り込みツールです。
あれ？Logstashは？と思う方もいると思いますが、Logstashは、豊富な機能を持ってます。
@<chapref>{logstash_pipelines}で説明したGrokフィルダで複雑なログを取り込むことも可能ですし、Inputのデータソースを多種多様に選択することが可能です。
そのためLogstashを利用するには、学習コストもそれなりに発生します。手軽に利用しよう！というのは難しいツールです。

そこで活躍するのがBeatsです。
何が手軽かというと、設定ファイルがYAMLで全て完結するのです。
しかも、設定箇所も複数存在するわけではなく、最低限の設定で十分な機能を提供します。



== Beats Family

Beatsにはどんな種類があるのかを改めて記載します。

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


Filebeatを使用することで、Apache、Nginx、MySQLなどのログ収集・パースをすることが可能です。
また、Modules機能を利用するととで、データの収集からKibanaを用いたデータの可視化までを一貫で行うことが可能です。

=== Filebeatをインストール


Filebeatをインストールします。@<chapref>{logstash}でyumリポジトリの登録が完了していることを前提として進めます。


//list[beats-01][Filebeatsのインストール]{
sudo yum install filebeat
//}

=== Ingest Node Pluginをインストール


UserAgent、GeoIPの解析をするため、@<code>{Ingest Node Plugin}と@<code>{Ingest GeoIP plugin}をインストールします。


//list[beats-02][Ingest Node Pluginのインストール]{
/usr/share/elasticsearch/bin/elasticsearch-plugin install ingest-user-agent
//}

//list[beats-03][Ingest GeoIP pluginのインストール]{
/usr/share/elasticsearch/bin/elasticsearch-plugin install ingest-geoip
//}


インストールが完了したらElasticsearchを再起動します。


//list[beats-04][Elasticsaerchの再起動]{
sudo service elasticsearch restart
//}

今回は@<code>{Nginx Modules}を例にModulesを利用すると、どの位構築コストが減少するのかを
検証します。@<chapref>{logstash}でKibanaをインストールしている環境を引き続き利用することを前提として
話を進めますが、もし新しい環境で始める場合は@<chapref>{logstash}や@<chapref>{Kibana-visualize}を参考に
環境構築を行なってください。


=== Nginx環境を整える

まず、Nginxをインストールします。

//list[beats-05][Nginxのインストール]{
sudo yum install nginx
//}

インストールが完了したら、Nginxを起動します。

//list[beats-06][Nginxの起動]{
sudo service nginx start
//}


Nginxに対してcurlを実行し、アクセスログが出力されているかを確認します。
また、ステータスコード200が返ってきていることも合わせて確認します。

//cmd{
$ tail -f /var/log/nginx/access.log
127.0.0.1 - - [xx/xxx/2018:xx:xx:xx +0000] "GET / HTTP/1.1" 200 3770 "-" "curl/7.53.1" "-"
//}

=== Filebeat Module

Filebeatの設定ファイルを編集する前に、@<code>{filebeat.yml}を@<code>{filebeat.reference.yml}に置き換えます。
このとき@<code>{filebeat.reference.yml}という名前から@<code>{filebeat.yml}に名前を合わせて変更します。
@<code>{filebeat.reference.yml}にModulesの設定が記載されているため、これを利用した方がより簡単にBeatsのセットアップが行えるからです。

//cmd{
### Change file name
mv /etc/filebeat/filebeat..yml /etc/filebeat/filebeat.yml_origin
mv /etc/filebeat/filebeat.reference.yml /etc/filebeat/filebeat.yml
//}


@<code>{filebeat.yml}の編集を行い、Nginxの有効化を行います。Nginxのパス設定ですが、インストールした状態（デフォルト）のまま
利用するのであればパスの変更は不要です。今回はデフォルト設定のまま利用しています。

//list[beats-07][filebeat.ymlの編集]{
#-------------------------------- Nginx Module -------------------------------
- module: nginx
  # Access logs
  access:
    enabled: true

    # Set custom paths for the log files. If left empty,
    # Filebeat will choose the paths depending on your OS.
    #var.paths:

    # Prospector configuration (advanced). Any prospector configuration option
    # can be added under this section.
    #prospector:

  # Error logs
  error:
    enabled: true

    # Set custom paths for the log files. If left empty,
    # Filebeat will choose the paths depending on your OS.
    #var.paths:

    # Prospector configuration (advanced). Any prospector configuration option
    # can be added under this section.
    #prospector:
//}


合わせて、OutputをElasticsearchに変更します。


//list[beats-08][Elasticsearchをデータ転送先にする]{
#-------------------------- Elasticsearch output -------------------------------
output.elasticsearch:
  # Boolean flag to enable or disable the output module.
  enabled: true

  # Array of hosts to connect to.
  # Scheme and port can be left out and will be set to the default (http and 9200)
  # In case you specify and additional path, the scheme is required: http://localhost:9200/path
  # IPv6 addresses should always be defined as: https://[2001:db8::1]:9200
  hosts: ["localhost:9200"]
//}


最後にKibanaのDashboardを起動時にセットアップする設定を有効化します。


//list[beats-09x][KibanaのDashboardを自動で作成する]{
### Activate Dashboards
#============================== Dashboards =====================================
# These settings control loading the sample dashboards to the Kibana index. Loading
# the dashboards are disabled by default and can be enabled either by setting the
# options here, or by using the `-setup` CLI flag or the `setup` command.
setup.dashboards.enabled: true
//}


@<code>{filebeat.reference.yml}をベースに@<code>{filebeat.yml}を作成しているため、
デフォルトでkafkaが@<code>{enabled: true}になっています。
このまま起動するとエラーが発生するためコメントアウトします。


//list[beats-10][kafkaのmodulesを利用しない]{
#-------------------------------- Kafka Module -------------------------------
#- module: kafka
  # All logs
  #log:
    #enabled: true
//}


では、いよいよFilebeatを起動します。


//list[beats-11][Filebeatの起動]{
sudo service filebeat start
//}


あとは、データが取り込まれているかをKibana@<href>{http://{Global_IP\}:5601}を開いて確認します。



以下のトップページが開きます。
左ペインにあるManagementをクリックします。


//image[filebeat01][Managementをクリック]{
//}

Index Patternsをクリックします。

//image[filebeat02][Indexを選択]{
//}

Filebeatのインデックスパターンが登録されていることがわかります。

//image[filebeat03][Filebeatのインデックスパターンを確認]{
//}

左ペインにある@<code>{Dashboard}をクリックします。
様々なDashboardが登録されていることがわかります。
Logstashなどでログを取り込んだ場合は、Dashboardを一から作成する必要がありますが、Beatsの場合は、あらかじめ用意されてます。

//image[filebeat04][Dashboardの確認]{
//}

今回は、Nginxの@<code>{Filebeat Nginx Overview}というDashboardをクリックします。
取り込んだログがDashboardに表示されていることがわかります。

//image[filebeat05][Filebeat Nginx Overview]{
//}

いかがでしたか？
他にも取り込みたいログがあれば、@<code>{filebeat.yml}のModuleを有効化するだけで容易にモニタリングができるようになります。



次は、サーバのリソースを容易にモニタリングを可能とする"Metricbeat"についてです。


== Metricbeat

Metricbeatは、サーバのリソース(CPU/Mem/process..など)を容易にモニタリングすることができます。
その他にもDockerやElasticsaerchなども対応しており、様々なプロダクトをモニタリングが可能です。

また、先ほどのFilebeatと同様にYAMLを編集するだけなので、学習コストもかかりません。
今回は、サーバのメトリックをモニタリングできるところまで見たいと思います。それでは、早速インストールしていきます。


//list[filebeat06][Metricbeatのインストール]{
sudo yum install metricbeat
//}


MetricbeatもFilebeat同様にベースの設定ファイル@<code>{metricbeat.reference.yml}があるのですが、
デフォルト有効化されているModuleが多いため、以下の設定ファイルを使用します。
既存で設定してある内容は全て上書きしてください。


//list[filebeat07][/etc/metricbeat/metricbeat.ymlの編集]{
##################### Metricbeat Configuration Example #######################

# This file is an example configuration file highlighting only the most common
# options. The metricbeat.reference.yml file from the same directory contains all the
# supported options with more comments. You can use it as a reference.
#
# You can find the full configuration reference here:
# https://www.elastic.co/guide/en/beats/metricbeat/index.html

#==========================  Modules configuration ============================
metricbeat.modules:
metricbeat.config.modules:
  # Glob pattern for configuration loading
  path: ${path.config}/modules.d/*.yml

  # Set to true to enable config reloading
  reload.enabled: false

  # Period on which files under path should be checked for changes
  #reload.period: 10s

#------------------------------- System Module -------------------------------
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

  # Configure the metric types that are included by these metricsets.
  cpu.metrics:  ["percentages"]  # The other available options are normalized_percentages and ticks.
  core.metrics: ["percentages"]  # The other available option is ticks.

  # A list of filesystem types to ignore. The filesystem metricset will not
  # collect data from filesystems matching any of the specified types, and
  # fsstats will not include data from these filesystems in its summary stats.
  #filesystem.ignore_types: []

  # These options allow you to filter out all processes that are not
  # in the top N by CPU or memory, in order to reduce the number of documents created.
  # If both the `by_cpu` and `by_memory` options are used, the union of the two sets
  # is included.
  #process.include_top_n:
    #
    # Set to false to disable this feature and include all processes
    #enabled: true

#==================== Elasticsearch template setting ==========================

setup.template.settings:
  index.number_of_shards: 1
  index.codec: best_compression
  #_source.enabled: false

#================================ General =====================================

# The name of the shipper that publishes the network data. It can be used to group
# all the transactions sent by a single shipper in the web interface.
#name:

# The tags of the shipper are included in their own field with each
# transaction published.
#tags: ["service-X", "web-tier"]

# Optional fields that you can specify to add additional information to the
# output.
#fields:
#  env: staging


#============================== Dashboards =====================================
# These settings control loading the sample dashboards to the Kibana index. Loading
# the dashboards is disabled by default and can be enabled either by setting the
# options here, or by using the `-setup` CLI flag or the `setup` command.
setup.dashboards.enabled: true

# The URL from where to download the dashboards archive. By default this URL
# has a value which is computed based on the Beat name and version. For released
# versions, this URL points to the dashboard archive on the artifacts.elastic.co
# website.
#setup.dashboards.url:

#============================== Kibana =====================================

# Starting with Beats version 6.0.0, the dashboards are loaded via the Kibana API.
# This requires a Kibana endpoint configuration.
setup.kibana:

  # Kibana Host
  # Scheme and port can be left out and will be set to the default (http and 5601)
  # In case you specify and additional path, the scheme is required: http://localhost:5601/path
  # IPv6 addresses should always be defined as: https://[2001:db8::1]:5601
  #host: "localhost:5601"

#================================ Outputs =====================================

# Configure what output to use when sending the data collected by the beat.

#-------------------------- Elasticsearch output ------------------------------
output.elasticsearch:
  # Array of hosts to connect to.
  hosts: ["localhost:9200"]

  # Optional protocol and basic auth credentials.
  #protocol: "https"
  #username: "elastic"
  #password: "changeme"

#================================ Logging =====================================

# Sets log level. The default log level is info.
# Available log levels are: error, warning, info, debug
#logging.level: debug

# At debug level, you can selectively enable logging only for some components.
# To enable all selectors use ["*"]. Examples of other selectors are "beat",
# "publish", "service".
#logging.selectors: ["*"]
//}

設定が完了したのでMetricbeatを起動します。


//list[filebeat08][Metricbeatの起動]{
sudo service metricbeat start
//}


Filebeatと同様にデータが取り込まれているかをKibanaを開いて確認します。
ブラウザを開いてKibana（@<href>{http://{Global_IP\}:5601}）へアクセスします。

Index Patternsの画面を開くとFilebeatのインデックスパターンの他にMetricbeatのインデックスパターンがあることがわかります

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
次が最後ですが、監査ログを容易に取り込むためのAuditbeatについてみていきます。


== Auditbeat

サーバの監査としてauditdが出力する@<code>{audit.log}をモニタリングしている方は多くいるのではないでしょうか。
@<code>{audit.log}を保管するだけでなく、ニアリアルタイムにモニタリングするためにLogstashなどのツールを利用している方もいると思います。
ただ、これから@<code>{audit.log}をモニタリングしたいという人からしたらハードルが高く、モニタリングするまでに時間を要してしまいます。
そこで、Beatsには、Auditbeatというデータシッパーがあるので容易に導入することができます。
ここまでFilbeatやMetricbeatを触ってきたらわかる通り、学習コストはほぼかからないでDashboardで閲覧するところまでできてしまいます。

それでは、ここからAuditbeatをインストールします。


//list[beats-09][Auditbeatのインストール]{
sudo yum install auditbeat
//}

@<list>{beats-10}の@<code>{auditbeat.yml}を既存で設定してある内容は全て上書きしてください。


//list[beats-10x][/etc/auditbeat/auditbeat.ymlの編集]{

###################### Auditbeat Configuration Example #########################

# This is an example configuration file highlighting only the most common
# options. The auditbeat.reference.yml file from the same directory contains all
# the supported options with more comments. You can use it as a reference.
#
# You can find the full configuration reference here:
# https://www.elastic.co/guide/en/beats/auditbeat/index.html

#==========================  Modules configuration =============================
auditbeat.modules:

- module: auditd
  audit_rules: |
    ## Define audit rules here.
    ## Create file watches (-w) or syscall audits (-a or -A). Uncomment these
    ## examples or add your own rules.

    ## If you are on a 64 bit platform, everything should be running
    ## in 64 bit mode. This rule will detect any use of the 32 bit syscalls
    ## because this might be a sign of someone exploiting a hole in the 32
    ## bit API.
    #-a always,exit -F arch=b32 -S all -F key=32bit-abi

    ## Executions.
    #-a always,exit -F arch=b64 -S execve,execveat -k exec

    ## External access (warning: these can be expensive to audit).
    #-a always,exit -F arch=b64 -S accept,bind,connect -F key=external-access

    ## Identity changes.
    #-w /etc/group -p wa -k identity
    #-w /etc/passwd -p wa -k identity
    #-w /etc/gshadow -p wa -k identity

    ## Unauthorized access attempts.
    #-a always,exit -F arch=b64 -S open,creat,truncate,ftruncate,openat,open_by_handle_at -F exit=-EACCES -k access
    #-a always,exit -F arch=b64 -S open,creat,truncate,ftruncate,openat,open_by_handle_at -F exit=-EPERM -k access

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
  #index.codec: best_compression
  #_source.enabled: false

#================================ General =====================================

# The name of the shipper that publishes the network data. It can be used to group
# all the transactions sent by a single shipper in the web interface.
#name:

# The tags of the shipper are included in their own field with each
# transaction published.
#tags: ["service-X", "web-tier"]

# Optional fields that you can specify to add additional information to the
# output.
#fields:
#  env: staging


#============================== Dashboards =====================================
# These settings control loading the sample dashboards to the Kibana index. Loading
# the dashboards is disabled by default and can be enabled either by setting the
# options here, or by using the `-setup` CLI flag or the `setup` command.
setup.dashboards.enabled: true

# The URL from where to download the dashboards archive. By default this URL
# has a value which is computed based on the Beat name and version. For released
# versions, this URL points to the dashboard archive on the artifacts.elastic.co
# website.
#setup.dashboards.url:

#============================== Kibana =====================================

# Starting with Beats version 6.0.0, the dashboards are loaded via the Kibana API.
# This requires a Kibana endpoint configuration.
setup.kibana:

  # Kibana Host
  # Scheme and port can be left out and will be set to the default (http and 5601)
  # In case you specify and additional path, the scheme is required: http://localhost:5601/path
  # IPv6 addresses should always be defined as: https://[2001:db8::1]:5601
  #host: "localhost:5601"

#================================ Outputs =====================================

# Configure what output to use when sending the data collected by the beat.

#-------------------------- Elasticsearch output ------------------------------
output.elasticsearch:
  # Boolean flag to enable or disable the output module.
  enabled: true
  # Array of hosts to connect to.
  hosts: ["localhost:9200"]

  # Optional protocol and basic auth credentials.
  #protocol: "https"
  #username: "elastic"
  #password: "changeme"

#================================ Logging =====================================

# Sets log level. The default log level is info.
# Available log levels are: error, warning, info, debug
#logging.level: debug

# At debug level, you can selectively enable logging only for some components.
# To enable all selectors use ["*"]. Examples of other selectors are "beat",
# "publish", "service".
#logging.selectors: ["*"]
//}


設定が完了したので、Auditbeatを起動します。


//list[beats-11x][Auditbeatの起動]{
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


= まとめ


いかがでしたか？
LogstashとBeatsの両方を操作できると、ログ収集時の選択肢が増えます。
2つのプロダクトの違いを理解できると、状況に合わせて適切なプロダクトの使い分けが可能となります。
みなさんがログと素敵な時間を過ごせることを願っています。

@micci184
