# Beats

Beatsは、シンプルなデータ取り込みツールです。  
あれ？Logstashは？と思う方もいると思いますが、Logstashは、豊富な機能を持ってます。  
前回の章で説明したGrokフィルダで複雑なログを取り込むことも可能ですし、"Input"のデータソースを多種多様に選択することが可能です。  
そのため、Logstashを利用するには、学習コストもそれなりに発生するので、手軽に利用することができません。  

そこで、手軽にデータを取り込みたい時に利用するのがBeatsです。  
何が手軽かというとYAMLで完結するのです。  
しかもほぼ設定する箇所はないです。

## Beats Family

それでは、Beats Familyを以下に記載します。

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

## Filebeat

Filebeatを使用することで、Apache、Nginx、MySQLなどのログ収集、パースが容易にできます。  
また、KibanaのDashboardも生成するため、すぐにモニタリングを始めることができます。

### Filebeatをインストール

Filebeatのインストールします。

```bash
### Install Filebeat
$ yum install filebeat
$ /usr/share/filebeat/bin/filebeat --version
Flag --version has been deprecated, version flag has been deprecated, use version subcommand
filebeat version 6.2.2 (amd64), libbeat 6.2.2
```

### Ingest Node Pluginをインストール

UserAgent、GeoIP解析をするため、以下のプラグインをインストールします。

```bash
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
```

問題なくインストールが完了したらElasticsearchを再起動します。

```bash
$ service elasticsearch restart
Stopping elasticsearch:                                    [  OK  ]
Starting elasticsearch:                                    [  OK  ]
```

FilebeatのNginx Moduleを使用して、どれだけ楽に構築できるかを触れたいと思います。  
そのほかのModuleについては、以下の公式ページに記載してあります。

> Filebeat Module: 
> https://www.elastic.co/guide/en/beats/filebeat/current/filebeat-modules.html

### Kibanaをインストール

KibanaのDashboardで取り込んだログを確認するところまで見るため、Kibanaをインストールします。  

```bash
### Install Kibana
$ yum install kibana
```

Kibanaへのアクセス元の制限をしないため、"server.host"の設定を変更します。[^1]

[^1]: AWSのSecurityGroup側で制限はかけているので、Kibana側では制限しないようにしています

```bash
### Change server.host
$ vim /etc/kibana/kibana.yml
server.host: 0.0.0.0
```

### Nginx環境を整える

Nginxをインストールし、Nginxのトップページが開くところまで実施します。

```bash
### Install Nginx
$ yum install nginx
### Start Nginx
$ service nginx start
Starting nginx:                                            [  OK  ]
```

curlを実行し、アクセスログが出力されているかを確認します。  
また、ステータスコード200が返ってきていることを確認します。

```bash
### Check access.log
$ tail -f /var/log/nginx/access.log
127.0.0.1 - - [xx/xxx/2018:xx:xx:xx +0000] "GET / HTTP/1.1" 200 3770 "-" "curl/7.53.1" "-"
```

### Filebeat Module

Filebeatの設定ファイルを編集する前に、"filebeat.yml"のファイル置き換えとファイル名変更を行います。  
理由は、"filebeat.reference.yml"にすべてのModuleなどが記載されているため、簡易的に利用できるためです。

```bash
### Change file name
mv /etc/filebeat/filebeat..yml /etc/filebeat/filebeat.yml_origin
mv /etc/filebeat/filebeat.reference.yml /etc/filebeat/filebeat.yml
```

"filebeat.yml"の編集を行い、Nginxの有効化、"Output"をElasticsearchに設定を行います。  
また、起動時にKibanaのDashboardを作成するよう設定します。   

"filebeat.yml"でNginxのModuleを有効化します。  
ログのパスはデフォルトから変更してなければ、変更不要です。  
今回は、デフォルトから変更していないため、変更しません。

```
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
```

"Output"をElasticsearchにするため、有効化します。

```bash
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
```

最後にKibanaのDashboardを起動時にセットアップする設定を有効化します。

```bash
### Activate Dashboards
#============================== Dashboards =====================================
# These settings control loading the sample dashboards to the Kibana index. Loading
# the dashboards are disabled by default and can be enabled either by setting the
# options here, or by using the `-setup` CLI flag or the `setup` command.
setup.dashboards.enabled: true
```

設定が完了したらFilebeatを起動します。

```bash
### Start Filebeat
service filebeat start
Starting filebeat: 2018-xx-xxTxx:xx:xx.140Z	INFO	instance/beat.go:468	Home path: [/usr/share/filebeat] Config path: [/etc/filebeat] Data path: [/var/lib/filebeat] Logs path: [/var/log/filebeat]
2018-xx-xxTxx:xx:xx.xxxZ	INFO	instance/beat.go:475	Beat UUID: e54958f0-6705-4586-8f9f-1d3599e568c0
2018-xx-xxTxx:xx:xx.xxxZ	INFO	instance/beat.go:213	Setup Beat: filebeat; Version: 6.2.2
2018-xx-xxTxx:xx:xx.xxxZ	INFO	elasticsearch/client.go:145	Elasticsearch url: http://localhost:9200
2018-xx-xxTxx:xx:xx.xxxZ	INFO	pipeline/module.go:76	Beat name: ip-172-31-50-36
2018-xx-xxTxx:xx:xx.xxxZ	INFO	beater/filebeat.go:62	Enabled modules/filesets: nginx (access, error), osquery (result),  ()
Config OK
                                                           [  OK  ]
```

あとは、データが取り込まれているかをKibanaを開いて確認します。  

ブラウザを開いてKibanaへアクセスします。

> http://{Global_IP}:5601

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


