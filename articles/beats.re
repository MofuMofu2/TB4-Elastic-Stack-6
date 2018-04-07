
= Beats


Beatsは、シンプルなデータ取り込みツールです。
あれ？Logstashは？と思う方もいると思いますが、Logstashは、豊富な機能を持ってます。
@<chapref>{logstash_pipelines}で説明したGrokフィルダで複雑なログを取り込むことも可能ですし、Inputのデータソースを多種多様に選択することが可能です。
そのためLogstashを利用するには、学習コストもそれなりに発生します。手軽に利用しよう！というのは難しいツールです。

そこで活躍するのがBeatsです。
何が手軽かというと、設定ファイルがYAMLで全て完結するのです。
しかも、設定箇所も複数存在するわけではなく、最低限の設定で十分な機能を提供します。



== Beats Family


冒頭の章で記載しましたが、改めてBeats Familyを以下に記載します。

 * Filebeat
 * Metricbeat
 * Packetbeat
 * Winlogbeat
 * Auditbeat
 * Heartbeat



この中でも以下のBeatsに触れていきたいと思います。

 * Filebeat
 * Metricbeat
 * Auditbeat


== Filebeat


Filebeatを使用することで、Apache、Nginx、MySQLなどのログ収集、パースが容易にできます。
また、KibanaのDashboardも生成するため、すぐにモニタリングを始めることができます。


=== Filebeatをインストール


Filebeatのインストールします。


//emlist[][bash]{
### Install Filebeat
$ yum install filebeat
$ /usr/share/filebeat/bin/filebeat --version
Flag --version has been deprecated, version flag has been deprecated, use version subcommand
filebeat version 6.2.2 (amd64), libbeat 6.2.2
//}

=== Ingest Node Pluginをインストール


UserAgent、GeoIP解析をするため、以下のプラグインをインストールします。


//emlist[][bash]{
### Install ingest-user-agent
$ /usr/share/elasticsearch/bin/elasticsearch-plugin install ingest-user-agent
-> Downloading ingest-user-agent from elastic
[=================================================] 100%
-> Installed ingest-user-agent

### Install ingest-geoip
$ /usr/share/elasticsearch/bin/elasticsearch-plugin install ingest-geoip
-> Downloading ingest-geoip from elastic
[=================================================] 100%
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@     WARNING: plugin requires additional permissions     @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
* java.lang.RuntimePermission accessDeclaredMembers
* java.lang.reflect.ReflectPermission suppressAccessChecks
See http://docs.oracle.com/javase/8/docs/technotes/guides/security/permissions.html
for descriptions of what these permissions allow and the associated risks.

Continue with installation? [y/N]y
-> Installed ingest-geoip
//}


問題なくインストールが完了したらElasticsearchを再起動します。


//emlist[][bash]{
$ service elasticsearch restart
Stopping elasticsearch:                                    [  OK  ]
Starting elasticsearch:                                    [  OK  ]
//}


FilebeatのNginx Moduleを使用して、どれだけ楽に構築できるかを触れたいと思います。
そのほかのModuleについては、以下の公式ページに記載してあります。


//quote{
Filebeat Module:
https://www.elastic.co/guide/en/beats/filebeat/current/filebeat-modules.html

//}

=== Kibanaをインストール


KibanaのDashboardで取り込んだログを確認するところまで見るため、Kibanaをインストールします。


//emlist[][bash]{
### Install Kibana
$ yum install kibana
//}


Kibanaへのアクセス元の制限をしないため、"server.host"の設定を変更します。@<fn>{1}


//emlist[][bash]{
### Change server.host
$ vim /etc/kibana/kibana.yml
server.host: 0.0.0.0
//}

=== Nginx環境を整える


Nginxをインストールし、Nginxのトップページが開くところまで実施します。


//emlist[][bash]{
### Install Nginx
$ yum install nginx
### Start Nginx
$ service nginx start
Starting nginx:                                            [  OK  ]
//}


curlを実行し、アクセスログが出力されているかを確認します。
また、ステータスコード200が返ってきていることを確認します。


//emlist[][bash]{
### Check access.log
$ tail -f /var/log/nginx/access.log
127.0.0.1 - - [xx/xxx/2018:xx:xx:xx +0000] "GET / HTTP/1.1" 200 3770 "-" "curl/7.53.1" "-"
//}

=== Filebeat Module


Filebeatの設定ファイルを編集する前に、"filebeat.yml"のファイル置き換えとファイル名変更を行います。
理由は、"filebeat.reference.yml"にすべてのModuleなどが記載されているため、簡易的に利用できるためです。


//emlist[][bash]{
### Change file name
mv /etc/filebeat/filebeat..yml /etc/filebeat/filebeat.yml_origin
mv /etc/filebeat/filebeat.reference.yml /etc/filebeat/filebeat.yml
//}


"filebeat.yml"の編集を行い、Nginxの有効化、"Output"をElasticsearchに設定を行います。
また、起動時にKibanaのDashboardを作成するよう設定します。



"filebeat.yml"でNginxのModuleを有効化します。
ログのパスはデフォルトから変更してなければ、変更不要です。
今回は、デフォルトから変更していないため、変更しません。


//emlist{
### Activate Nginx module
$vim /etc/filebeat/filebeat.yml
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


"Output"をElasticsearchにするため、有効化します。


//emlist[][bash]{
### Activate Elasticsearch output
$vim /etc/filebeat/filebeat.yml
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


//emlist[][bash]{
### Activate Dashboards
#============================== Dashboards =====================================
# These settings control loading the sample dashboards to the Kibana index. Loading
# the dashboards are disabled by default and can be enabled either by setting the
# options here, or by using the `-setup` CLI flag or the `setup` command.
setup.dashboards.enabled: true
//}


"filebeat.reference.yml"をベースに作成しているため、デフォルトでkafkaが"enabled: true"になっています。
このまま起動するとエラーが発生するためコメントアウトします。


//emlist[][bash]{
### Comment out kafka module
$ vim /etc/filebeat/filebeat.yml
#-------------------------------- Kafka Module -------------------------------
#- module: kafka
  # All logs
  #log:
    #enabled: true
//}


設定が完了したらFilebeatを起動します。


//emlist[][bash]{
### Start Filebeat
service filebeat start
Starting filebeat: 2018-xx-xxTxx:xx:xx.xxxZ INFO    instance/beat.go:468    Home path: [/usr/share/filebeat] Config path: [/etc/filebeat] Data path: [/var/lib/filebeat] Logs path: [/var/log/filebeat]
2018-xx-xxTxx:xx:xx.xxxZ    INFO    instance/beat.go:475    Beat UUID: e54958f0-6705-4586-8f9f-1d3599e568c0
2018-xx-xxTxx:xx:xx.xxxZ    INFO    instance/beat.go:213    Setup Beat: filebeat; Version: 6.2.2
2018-xx-xxTxx:xx:xx.xxxZ    INFO    elasticsearch/client.go:145 Elasticsearch url: http://localhost:9200
2018-xx-xxTxx:xx:xx.xxxZ    INFO    pipeline/module.go:76   Beat name: ip-172-31-50-36
2018-xx-xxTxx:xx:xx.xxxZ    INFO    beater/filebeat.go:62   Enabled modules/filesets: nginx (access, error), osquery (result),  ()
Config OK
                                                           [  OK  ]
//}


あとは、データが取り込まれているかをKibanaを開いて確認します。



ブラウザを開いてKibanaへアクセスします。


//quote{
http://{Global_IP}:5601

//}


以下のトップページが開きます。
左ペインにある"Management"をクリックします。



[filebeat01.png]



"Index Patterns"をクリックします。



[filebeat02.png]



Filebeatのインデックスパターンが登録されていることがわかります。



[filebeat03.png]



左ペインにある"Dashboard"をクリックします。
様々なDashboardが登録されていることがわかります。
Logstashなどでログを取り込んだ場合は、Dashboardを一から作成する必要がありますが、Beatsの場合は、あらかじめ用意されてます。



[filebeat04.png]



今回は、Nginxの"[Filebeat Nginx] Overview"というDashboardをクリックします。
取り込んだログがDashboardに表示されていることがわかります。



[filebeat05.png]



いかがでしたか？
他にも取り込みたいログがあれば、"filebeat.yml"のModuleを有効化するだけで容易にモニタリングができるようになります。



次は、サーバのリソースを容易にモニタリングを可能とする"Metricbeat"についてです。


== Metricbeat


Metricbeatは、サーバのリソース(CPU/Mem/process..etc)を容易にモニタリングすることができます。
その他にもDockerやElasticsaerchなども対応しており、様々なプロダクトをモニタリングが可能です。



また、先ほどのFilebeatと同様にYAMLを編集するだけなので、学習コストもほぼいらずに導入できます。
今回は、サーバのメトリックをモニタリングできるところまで見たいと思います。



それでは、早速インストールしていきます。


//emlist[][bash]{
### Install Metricbeat
$ yum install metricbeat
//}


MetricbeatもFilebeat同様にベースの設定ファイル(metricbeat.reference.yml)があるのですが、デフォルト有効化されているModuleが多いため、以下の設定ファイルを使用します。
既存で設定してある内容は全て上書きしてください。


//emlist[][bash]{
### Create metricbeat.yml
$ vim /etc/metricbeat/metricbeat.yml
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


//emlist[][bash]{
### Start Metricbeat
$ service metricbeat start
Starting metricbeat: 2018-xx-xxTxx:xx:xx.xxxZ   INFO    instance/beat.go:468    Home path: [/usr/share/metricbeat] Config path: [/etc/metricbeat] Data path: [/var/lib/metricbeat] Logs path: [/var/log/metricbeat]
2018-xx-xxTxx:xx:xx.xxxZ    INFO    instance/beat.go:475    Beat UUID: 133de8d7-18b1-472e-ac24-79831b9203cf
2018-xx-xxTxx:xx:xx.xxxZ    INFO    instance/beat.go:213    Setup Beat: metricbeat; Version: 6.2.2
2018-xx-xxTxx:xx:xx.xxxZ    INFO    elasticsearch/client.go:145 Elasticsearch url: http://localhost:9200
2018-xx-xxTxx:xx:xx.xxxZ    INFO    pipeline/module.go:76   Beat name: ip-172-31-50-36
2018-xx-xxTxx:xx:xx.xxxZ WARN   [cfgwarn]   socket/socket.go:49 BETA: The system collector metricset is beta
Config OK
                                                           [  OK  ]
//}


Filebeatと同様にデータが取り込まれているかをKibanaを開いて確認します。
ブラウザを開いてKibanaへアクセスします。


//quote{
http://{Global_IP}:5601

//}


"Index Patterns"の画面を開くとFilebeatのインデックスパターンの他にMetricbeatのインデックスパターンがあることがわかります



[metricbeat01.png]



左ペインにある"Dashboard"をクリックします。
検索ウィンドウから"Metricbeat"を入力すると様々なDashboardがヒットします。



[metricbeat02.png]



今回は、"[Metricbeat System] Host Overview"というDashboardをクリックします。
CPUやメモリ、プロセスの状態をニアリアルタイムにモニタリングができていることがわかります。



[metricbeat03.png]



このようにサーバやコンテナなどにMetricbeatを導入することで一元的にモニタリングすることができます。
次が最後ですが、監査ログを容易に取り込むための"Auditbeat"についてです。


== Auditbeat


サーバの監査としてauditdが出力する"audit.log"をモニタリングしている方は多くいるのではないでしょうか。
"audit.log"を保管するだけでなく、ニアリアルタイムにモニタリングするためにLogstashなどのツールを利用している方もいると思います。
ただ、これから"audit.log"をモニタリングしたいという人からしたらハードルが高く、モニタリングするまでに時間を要してしまいます。
そこで、Beatsには、Auditbeatというデータシッパーがあるので容易に導入することができます。
ここまでFilbeatやMetricbeatを触ってきたらわかる通り、学習コストはほぼかからないでDashboardで閲覧するところまでできてしまいます。



それでは、ここからAuditbeatをインストールします。


//emlist[][bash]{
### Install Auditbeat
$ yum install auditbeat
//}


Auditbeatの設定ファイルは、以下を使用します。
既存で設定してある内容は全て上書きしてください。


//emlist[][bash]{
### Create auditbeat.yml
$ vim /etc/auditbeat/auditbeat.yml
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


//emlist[][bash]{
### Start Auditbeat
$ service auditbeat start
Starting auditbeat: 2018-xx-xxTxx:xx:xx.xxxZ    INFO    instance/beat.go:468    Home path: [/usr/share/auditbeat] Config path: [/etc/auditbeat] Data path: [/var/lib/auditbeat] Logs path: [/var/log/auditbeat]
2018-xx-xxTxx:xx:xx.xxxZ    INFO    instance/beat.go:475    Beat UUID: c8ed5e31-a553-4c66-a69d-401d9bf38c18
2018-xx-xxTxx:xx:xx.xxxZ    INFO    instance/beat.go:213    Setup Beat: auditbeat; Version: 6.2.2
2018-xx-xxTxx:xx:xx.xxxZ    INFO    elasticsearch/client.go:145 Elasticsearch url: http://localhost:9200
2018-xx-xxTxx:xx:xx.xxxZ    INFO    pipeline/module.go:76   Beat name: ip-172-31-50-36
2018-xx-xxTxx:xx:xx.xxxZ    INFO    [auditd]    auditd/audit_linux.go:65    auditd module is running as euid=0 on kernel=4.9.76-3.78.amzn1.x86_64
2018-xx-xxTxx:xx:xx.xxxZ    INFO    [auditd]    auditd/audit_linux.go:88    socket_type=multicast will be used.
Config OK
                                                           [  OK  ]
//}


データが取り込まれているかをKibanaを開いて確認します。
ブラウザを開いてKibanaへアクセスします。


//quote{
http://{Global_IP}:5601

//}


"Index Patterns"の画面を開くとFilebeatのインデックスパターンの他にAuditbeatのインデックスパターンがあることがわかります



[auditbeat01.png]



左ペインにある"Dashboard"をクリックします。
検索ウィンドウから"Auditbeat"を入力すると様々なDashboardがヒットします。



[auditbeat02.png]



"[Auditbeat File Integrity] Overview"や"[Auditbeat Auditd] Overview"からモニタリングが可能です。



[auditbeat03.png]



これまでBeatsを見てきていかがでしたか？
モニタリングしたいModuleを有効化するだけで容易にモニタリングできる環境が手に入ります。
この他にもWidnowsを対象にしたものや、サービスの死活監視としてのBeatsなどがあります。
同じような学習コストで体験できるので、体験して頂ければと思います。


= まとめ


いかがでしたか？
LogstashとBeatsの両方を体験することで、ログ収集時の選択肢が増えたのではないでしょうか。
また、LogstashとBeatsの違いがわからないという方々にお会いすることがあるので、少しでも違いを理解頂ければ幸いです。



最後となりますが、ここまでお付き合い頂きありがとうございます。



これからもみなさんがログと素敵な時間を過ごせることを願ってます。



@micci184


//footnote[1][AWSのSecurityGroup側で制限はかけているので、Kibana側では制限しないようにしています]
