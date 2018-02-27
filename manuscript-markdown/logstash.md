# Logstashを使ってみる

Logstashは、"Input""Filter""Output"の構成で記述します。  
Inputでログのソースを指定し、Filterで必要な要素に分解します。最後のOutputで出力先を指定します。  
今回は、AWS環境なので、Inputは、S3からログを取得して、OutputでElasticsearchにストアさせます。  

それでは、ここから実際にLogstashをインストールして、どのようなことを行うかを見ていきたいと思います。  

##  実行環境を準備するよ

Logstashのインストールは以下の公式でも記載されてるのですが、英語です。  
やはり抵抗感を覚える人もいると思うので、できる限りわかりやすく日本語で書きます。  
正直、英語ぐらいわかるわー！って人は、飛ばしちゃってください。  

> Install Logstash: https://www.elastic.co/guide/en/logstash/current/installing-logstash.html

### Java 8インストール

Logstashをインストールするにあたり、Java 8が必要なので、インストールします。
Javaの切り替えにalternativesコマンドを使用して変更します。

```bash
### Install Java 8
$ sudo yum -y install java-1.8.0-openjdk-devel
$ sudo alternatives --config java

There are 2 programs which provide 'java'.

  Selection    Command
-----------------------------------------------
*+ 1           /usr/lib/jvm/jre-1.7.0-openjdk.x86_64/bin/java
   2           /usr/lib/jvm/jre-1.8.0-openjdk.x86_64/bin/java

Enter to keep the current selection[+], or type selection number: 2
$ java -version
openjdk version "1.8.0_161"
OpenJDK Runtime Environment (build 1.8.0_161-b14)
OpenJDK 64-Bit Server VM (build 25.161-b14, mixed mode)
```

### GPGキーをインポート

```bash
### Import GPG-Key
$ rpm --import https://artifacts.elastic.co/GPG-KEY-elasticsearch
```

### YUMリポジトリの追加

logstashのパッケージを取得するため、YUMリポジトリを追加します。  
"/etc/yum.repo/"配下に"elasticstack.repo"というファイルを作成します。  
公式では、logstash.repoとなっておりますが、今回はElasticsearchなどもインストールするため、汎用的な名前にしました。要は任意で問題ないということです。

```bash
### Add elastic.repo
$ sudo vim /etc/yum.repos.d/elasticstack.repo
[elasticstack-6.x]
name=Elastic repository for 6.x packages
baseurl=https://artifacts.elastic.co/packages/6.x/yum
gpgcheck=1
gpgkey=https://artifacts.elastic.co/GPG-KEY-elasticsearch
enabled=1
autorefresh=1
type=rpm-md
```

### Logstashをインストール

最後にLogstashをインストールします。

```bash
### Install Logstash
$ sudo yum install logstash
$ /usr/share/logstash/bin/logstash --version
logstash 6.2.2
```

S3をデータソースにするため、"S3 Input Plugin"をインストールします。

> S3 Input Plugin: https://www.elastic.co/guide/en/logstash/current/plugins-inputs-s3.html

```bash
$ /usr/share/logstash/bin/logstash-plugin install logstash-input-s3
Validating logstash-input-s3
Installing logstash-input-s3
Installation successful
```

### Elasticsearchをインストール

Output先としてElasticsearchを利用するため、Elasticsearchをインストールします。

```bash
### Install Elasticsearch
$ sudo yum install elasticsearch
$ /usr/share/elasticsearch/bin/elasticsearch --version
Version: 6.2.2, Build: 10b1edd/2018-02-16T19:01:30.685723Z, JVM: 1.8.0_161
``` 

サービスの自動起動をする場合は、以下を実行してください。

```bash
### Auto start setting
$ sudo chkconfig --add elasticsearch
$ chkconfig --list | grep elasticsearch
elasticsearch  	0:off	1:off	2:on	3:on	4:on	5:on	6:off
```

## Elasticsearchの準備するよ

ここからはElasticsearchの設定ファイルの編集をします。

### Elasticsearchのディレクトリ構成について

Elasticsearchのディレクトリ構成は以下です。  
elasticsearch.ymlとjvm.optionsの設定を編集します。
ログの出力部分など編集したい場合は、log4j2.propertiesを編集してください。

```bash
### Elasticsearch directory structure
/etc/elasticsearch/
 ┣ elasticsearch.yml
 ┣ jvm.options
 ┗ log4j2.properties
```

### jvm.optionsの編集

jvm.optionsからヒープサイズを変更することが可能で、"Xms(minimum heap size)"と"Xmx(maximum heap size) "を変更します。  
Elasticsearch社の公式に書かれている設定アドバイスは以下です。

* 最小ヒープサイズ(Xms)と最大ヒープサイズ(Xmx)の値を等しくする
* ヒープサイズを上げすぎるとGCの休止をまねく可能性がある
* Xmxは、物理メモリの50%を割り当てて、ファイルシステム側にも十分に残すようにする
* 割り当てるメモリは、32GB以下にする

上記を踏まえて"jvm.options"の設定をします。
インスタンスのメモリは、4GBなので2GBを割り当てます。

```bash
### Heap size change
$ vim /etc/elasticsearch/jvm.options
-Xms2g
-Xmx2g
```

> Settings the heap size:
> https://www.elastic.co/guide/en/elasticsearch/reference/current/heap-size.html

### elasticserch.ymlの編集

elasticsearch.ymlでノード名や、クラスタ名の編集が可能ですが、デフォルト値とします。  
今回は、どこからでもアクセス可能とするため、"network.host"のみを編集します。
"0.0.0.0"と設定することで、どこからでもアクセス可能となります。

```bash
### Settings to enable access from anywhere
$ network.host: 0.0.0.0
```

参考までに実運用では必要となりうる"elasticserch.yml"の設定値について記載します。

| No. | Item                               | Content                                                                |
|:----|:-----------------------------------|:-----------------------------------------------------------------------|
| 1   | cluster.name: my-application       | クラスタ名の指定                                                       |
| 2   | node.name                          | ノード名の指定                                                         |
| 3   | network.host                       | アクセス元のネットワークアドレスを指定することで制限をかけることが可能 |
| 4   | discovery.zen.ping.unicast.hosts   | クラスタを組む際にノードを指定                                         |
| 5   | discovery.zen.minimum_master_nodes | 必要最低限のノード数を指定                                             |

Elasticsearchを起動し、動作確認をします。
curlで実行し、レスポンスが返ってくれば問題なく起動してます。

```bash
### Service activation
$ service elasticsearch start
Starting elasticsearch:                                    [  OK  ]
### Check the operation of elasticsearch
$ curl localhost:9200
{
  "name" : "F5iPU0m",
  "cluster_name" : "elasticsearch",
  "cluster_uuid" : "Tc_Wn8RJRZ2wjAFsJkShAw",
  "version" : {
    "number" : "6.2.2",
    "build_hash" : "10b1edd",
    "build_date" : "2018-02-16T19:01:30.685723Z",
    "build_snapshot" : false,
    "lucene_version" : "7.2.1",
    "minimum_wire_compatibility_version" : "5.6.0",
    "minimum_index_compatibility_version" : "5.0.0"
  },
  "tagline" : "You Know, for Search"
}
```

これでElasticsearchの準備が完了です。

## Logstashの設定ファイルを触ってみる

Logstashのディレクトリ構成は以下です。

```bash
### Elasticsearch directory structure
/etc/logstash/
 ┣ conf.d
 ┣ jvm.options
 ┣ log4j2.properties
 ┣ logstash.yml
 ┣ pipelines.yml
 ┗ startup.options
```

各ファイルやディレクトリにについて説明します。

| No. | Item              | Content                                                       |
|:----|:------------------|:--------------------------------------------------------------|
| 1   | conf.d            | Input/Filter/Outputのパイプラインを記載したファイルの格納場所 |
| 2   | jvm.options       | jvmオプションの管理ファイル                                   |
| 3   | log4j2.properties | log4jのプロパティ管理ファイル                                 |
| 4   | logstash.yml      | Logstashの設定ファイル                                        |
| 5   | pipelines.yml     | パイプラインを複数実行する際に定義するファイル                |
| 6   | startup.options   | Logstash起動時に利用されるファイル                            |

### logstash.ymlの編集

今回は、logstash.ymlの編集は行いませんが、このファイルでどんなことができるかをサクッと書いておきます。  
logstash.ymlでは、パイプラインのバッチサイズやディレイ設定が可能です。  
例えば、以下のように階層やフラットな構造で記載することが可能です。

```bash
### hierarchical form
pipeline:
  batch:
    size: 125
    delay: 50
### flat key
pipeline.batch.size: 125
pipeline.batch.delay: 50
```

また、パイプラインを実行するWoker数を変更することも可能です。
変更する際は、"pipeline.workers"の数を変更します。
Woker数の目安は、割り当てたいCPUコア数とイコールにするのが良いです。

```bash
### Change Woker
pipeline.workers: 2
```

公式に詳細が記載されているので参考にすると幸せになれます。

> Settings File:
> https://www.elastic.co/guide/en/logstash/current/logstash-settings-file.html

### パイプラインを実行してみる

ここからLogstashのパイプラインを動かしていきたいと思います。  
まずは、単純にコマンドラインからLogstashを実行したいと思います。

パイプラインの設定ファイルを作成します。  
このパイプラインは、単純に標準入力から標準出力するものです。

```bash
### Cleate pipeline
$ vim /etc/logstash/conf.d/test.conf
input { 
  stdin { }
}
output {
  stdout { codec => rubydebug }
}
```

以下のコマンドを実行し、"Pipelines running"と表示されたら任意の文字を標準入力します。
入力した文字（ここではtest）が"message"に表示にされていることがわかります。

```bash
### Run Pipeline
$ /usr/share/logstash/bin/logstash -f /etc/logstash/conf.d/test.conf
.
.
.
[INFO ] 2018-xx-xx xx:xx:xx.xxx [Ruby-0-Thread-1: /usr/share/logstash/vendor/bundle/jruby/2.3.0/gems/stud-0.0.23/lib/stud/task.rb:22] agent - Pipelines running {:count=>1, :pipelines=>["main"]}
### Enter arbitrary characters
test
{
      "@version" => "1",
       "message" => "test",
    "@timestamp" => 2018-xx-xx xx:xx:xx.xxx,
          "host" => "ip-xxx-xxx-xxx-xxx"
}
```

### さらにパイプラインを実行してみる

ここからは実際のログを利用してパイプラインを扱っていきたいと思います。
対象のログとして、AWSのALBログを利用します。

AWS公式ページに記載されているサンプルログを利用します。

> Access Logs for Your Application Load Balancer: 
> https://docs.aws.amazon.com/elasticloadbalancing/latest/application/load-balancer-access-logs.html

```
https 2016-08-10T23:39:43.065466Z app/my-loadbalancer/50dc6c495c0c9188 
192.168.131.39:2817 10.0.0.1:80 0.086 0.048 0.037 200 200 0 57 
"GET https://www.example.com:443/ HTTP/1.1" "curl/7.46.0" ECDHE-RSA-AES128-GCM-SHA256 TLSv1.2 
arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067
"Root=1-58337281-1d84f3d73c47ec4e58577259" www.example.com arn:aws:acm:us-east-2:123456789012:certificate/12345678-1234-1234-1234-123456789012
```

このサンプルログを"/etc/logstash/"配下に配置します。
ログのファイル名を"alb.log"にします。

```bash
$ vim /etc/logstash/alb.log
https 2016-08-10T23:39:43.065466Z app/my-loadbalancer/50dc6c495c0c9188  192.168.131.39:2817 10.0.0.1:80 0.086 0.048 0.037 200 200 0 57 "GET https://www.example.com:443/ HTTP/1.1" "curl/7.46.0" ECDHE-RSA-AES128-GCM-SHA256 TLSv1.2  arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067 "Root=1-58337281-1d84f3d73c47ec4e58577259" www.example.com arn:aws:acm:us-east-2:123456789012:certificate/12345678-1234-1234-1234-123456789012
```

パイプラインの設定ファイルを作成します。
先程までは、"Input"を標準入力としてましたが、ファイルを取り込むので"File input plugin"を使用します。
標準でインストールされているので、インストールは不要です。

それでは、新しく"alb.conf"という設定ファイルを作成します。

```bash
$ vim /etc/logstash/conf.d/alb.conf
input {
  file{
    path=>"/etc/logstash/alb.log"
    start_position=>"beginning"
    sincedb_path => "/dev/null"
  }
}
output {
  stdout { codec => rubydebug }
}
```

追記した部分を以下で以下で説明します。

| No. | Item           | Content                                                         |
|:----|:---------------|:----------------------------------------------------------------|
| 1   | path           | 取り込むファイルを指定します(ディレクトリ指定の"\*"指定も可能)  |
| 2   | start_position | Logstash起動した時にどこから読み込むかの指定(デフォルトは、end) |
| 3   | sincedb_path   | ログファイルを前回どこまで取り込んだかを記載するファイル        |

ではでは、作った設定ファイルで実行します。

```bash
### Run Pipeline
$ /usr/share/logstash/bin/logstash -f /etc/logstash/conf.d/alb.conf
{
    "@timestamp" => 2018-02-26T08:15:31.322Z,
          "path" => "/etc/logstash/alb.logs",
       "message" => "https 2016-08-10T23:39:43.065466Z app/my-loadbalancer/50dc6c495c0c9188  192.168.131.39:2817 10.0.0.1:80 0.086 0.048 0.037 200 200 0 57 "GET https://www.example.com:443/ HTTP/1.1" "curl/7.46.0" ECDHE-RSA-AES128-GCM-SHA256 TLSv1.2  arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067 "Root=1-58337281-1d84f3d73c47ec4e58577259" www.example.com arn:aws:acm:us-east-2:123456789012:certificate/12345678-1234-1234-1234-123456789012,
      "@version" => "1",
          "host" => "ip-172-31-50-36"
}
```

標準入力で実行した時と同様に"message"に取り込んだログが出力されていることがわかります。
ただ、これでは構造化した形でElasticsearchにストアされないため、検索性が損なわれます。
その解決方法として"Filter"を利用します。

### Filterを使ってみる

"Filter"では何を行うかというと、取得したログを正規表現でパースするGrokフィルタや、地理情報を得るためのGioIPフィルタを施すことができます。
今回のALBもGrokフィルタなどを使うことで構造化することが可能です。

まず、ALBのログフォーマットを把握する必要があります。
以下に記載します。

```bash
type timestamp elb client:port target:port request_processing_time target_processing_time response_processing_time elb_status_code target_status_code received_bytes sent_bytes "request" "user_agent" ssl_cipher ssl_protocol target_group_arn trace_id domain_name chosen_cert_arn
```

各フィールドについて以下に記載します。  

| Log                      | Field                    | Type   |
|:-------------------------|:-------------------------|:-------|
| type                     | class                    | string |
| timestamp                | date                     | date   |
| elb                      | elb                      | string |
| client_ip                | client_ip                | int    |
| client_port              | target_port              | int    |
| target_ip                | target_ip                | int    |
| target_port              | target_port              | int    |
| request_processing_time  | request_processing_time  | float  |
| target_processing_time   | target_processing_time   | float  |
| response_processing_time | response_processing_time | float  |
| elb_status_code          | elb_status_code          | string |
| target_status_code       | target_status_code       | string |
| received_bytes           | received_bytes           | int    |
| sent_bytes               | sent_bytes               | int    |
| request                  | ELB_REQUEST_LINE         | string |
| user_agent               | user_agent               | string |
| ssl_cipher               | ssl_cipher               | string |
| ssl_protocol             | ssl_protocol             | string |
| target_group_arn         | target_group_arn         | string |
| trace_id                 | trace_id                 | string |

このフィールド単位でフィルタをかけられるようkey-valueにGrokフィルタで分解します。  
ALB用の"grok-patterns"を記載したパターンファイルを作成します。  

が、その前にパターンファイルを格納するディレクトリを作成します。
パイプラインの設定ファイルにGrokフィルタを記載するでもいいのですが、可読性を上げるため外だしにしてます。

```bash
### Create directory
$ mkdir /etc/logstash/patterns
$ ll | grep patterns
drwxr-xr-x 2 root root 4096 xxx xx xx:xx patterns
```

patternsディレクトリ配下にALBのパターンファイルを作成します。  
中身については、闇深いのでここでは説明しません。。無邪気に貼っつけてください。  
また、Typeは、インデックステンプレートで作成するのが一般的かと思いますが、今回は、パターンファイルの中で指定します。

```bash
$ sudo vim /etc/logstash/patterns/alb_patterns
# Application Load Balancing
ALB_ACCESS_LOG %{NOTSPACE:class} %{TIMESTAMP_ISO8601:date} %{NOTSPACE:elb}  (?:%{IP:client_ip}:%{INT:client_port:int}) (?:%{IP:backend_ip}:%{INT:backend_port:int}|-) (:?%{NUMBER:request_processing_time:float}|-1) (?:%{NUMBER:target_processing_time:float}|-1) (?:%{NUMBER:response_processing_time:float}|-1) (?:%{INT:elb_status_code}|-) (?:%{INT:target_status_code:int}|-) %{INT:received_bytes:int} %{INT:sent_bytes:int} \"%{ELB_REQUEST_LINE}\" \"(?:%{DATA:user_agent}|-)\" (?:%{NOTSPACE:ssl_cipher}|-) (?:%{NOTSPACE:ssl_protocol}|-)  %{NOTSPACE:target_group_arn} \"%{NOTSPACE:trace_id}\"
```

alb.confに"Fiter"を追加します。

```bash
### update alb.conf
$ vim /etc/logstash/conf.d/alb.conf
input {
  file{
    path=>"/etc/logstash/alb.log"
    start_position=>"beginning"
    sincedb_path => "/dev/null"
  }
}
filter {
  grok {
    patterns_dir => ["/etc/logstash/patterns/alb_patterns"]
    match => { "message" => "%{ALB_ACCESS_LOG}" }
  }
  date {
    match => [ "date", "ISO8601" ]
    timezone => "Asia/Tokyo"
    target => "@timestamp"
  }
  geoip {
    source => "client_ip"
  }
}
output {
  stdout { codec => rubydebug }
}
```

更新できたら実行します。
いい感じにkey-valueのかたちになっていることがわかります。

```bash
{
                        "verb" => "GET",
     "request_processing_time" => 0.086,
                  "sent_bytes" => 57,
                  "ssl_cipher" => "ECDHE-RSA-AES128-GCM-SHA256",
                   "client_ip" => "5.10.83.30",
                     "request" => "https://www.example.com:443/",
                       "proto" => "https",
                        "port" => "443",
                  "user_agent" => "curl/7.46.0",
                       "geoip" => {
             "city_name" => "Amsterdam",
              "location" => {
            "lon" => 4.9167,
            "lat" => 52.35
        },
              "timezone" => "Europe/Amsterdam",
                    "ip" => "5.10.83.30",
           "postal_code" => "1091",
         "country_code3" => "NL",
        "continent_code" => "EU",
             "longitude" => 4.9167,
          "country_name" => "Netherlands",
           "region_name" => "North Holland",
              "latitude" => 52.35,
         "country_code2" => "NL",
           "region_code" => "NH"
    },
          "target_status_code" => 200,
                 "client_port" => 2817,
                "backend_port" => 80,
                    "trace_id" => "Root=1-58337281-1d84f3d73c47ec4e58577259",
                       "class" => "https",
            "target_group_arn" => "arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067",
                     "urihost" => "www.example.com:443",
                        "path" => [
        [0] "/etc/logstash/alb.log",
        [1] "/"
    ],
                 "httpversion" => "1.1",
             "elb_status_code" => "200",
                        "host" => "ip-172-31-50-36",
                  "@timestamp" => 2016-08-10T23:39:43.065Z,
      "target_processing_time" => 0.048,
    "response_processing_time" => 0.037,
                         "elb" => "app/my-loadbalancer/50dc6c495c0c9188",
                "ssl_protocol" => "TLSv1.2",
                        "date" => "2016-08-10T23:39:43.065466Z",
                     "message" => "https 2016-08-10T23:39:43.065466Z app/my-loadbalancer/50dc6c495c0c9188  5.10.83.30:2817 10.0.0.1:80 0.086 0.048 0.037 200 200 0 57 \"GET https://www.example.com:443/ HTTP/1.1\" \"curl/7.46.0\" ECDHE-RSA-AES128-GCM-SHA256 TLSv1.2  arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067 \"Root=1-58337281-1d84f3d73c47ec4e58577259\" www.example.com arn:aws:acm:us-east-2:123456789012:certificate/12345678-1234-1234-1234-123456789012",
              "received_bytes" => 0,
                  "backend_ip" => "10.0.0.1",
                    "@version" => "1"
}
```

それでは、Filterで記載している内容について説明します。

正規表現でパースする際にgrokフィルタを使用します。
"patterns_dir"で外だししているパターンファイルを呼び出すことができます。
"match"で"message"に取り込まれている値を対象にGrok-Patterns(ここでいうALB_ACCESS_LOG)を適用しています。

```bash
### grok-filter
  grok {
    patterns_dir => ["/etc/logstash/patterns/alb_patterns"]
    match => { "message" => "%{ALB_ACCESS_LOG}" }
  }
```

dateフィルタで"@timestamp"をgrokフィルタで抽出した"date"を置き換えます。
また、タイムゾーンを指定することも可能です。

```bash
  date {
    match => [ "date", "ISO8601" ]
    timezone => "Asia/Tokyo"
    target => "@timestamp"
  }
```

geoipフォルタを使用することでIPアドレスから地理情報を取得することが可能です。
geoipフィルタの対象とするため、"client_ip"を指定してます。
"client_ip"を指定する意図は、どこの国からアクセスがきているかを把握するためです。

```bash
  geoip {
    source => "client_ip"
  }
```

今回は使用していないですが、不要な値は、mutateで削除可能です。
例えば、messageの値は、全てkey-valueでストアされているから不要なので削除といったことも可能です。
個人的には、ストアされたデータで"_grokparsefailure"が発生した時の場合も踏まえると、残した方がいいと思ってます。[^1]

[^1]: "_grokparsefailure"は、grokフィルタでパースできない場合に発生します

messageを削除する場合は、Filterにmutateを追加します。

```bash
filter {
  grok {
    patterns_dir => ["/etc/logstash/patterns/alb_patterns"]
    match => { "message" => "%{ALB_ACCESS_LOG}" }
  }
  date {
    match => [ "date", "ISO8601" ]
    timezone => "Asia/Tokyo"
    target => "@timestamp"
  }
  geoip {
    source => "client_ip"
  }
  ### mutateを追加し、remove_fieldでmessageを削除
  mutate {
    remove_field => [ "message" ]
  }  
}
```

補足ですが、コマンドラインで実行している際に以下のようなエラーが発生した場合は、Logstashのプロセスがすでに立ち上がっている時に発生します。

```bash
### Error executing logstash
$ /usr/share/logstash/bin/logstash -f conf.d/alb.conf
WARNING: Could not find logstash.yml which is typically located in $LS_HOME/config or /etc/logstash. You can specify the path using --path.settings. Continuing using the defaults
Could not find log4j2 configuration at path /usr/share/logstash/config/log4j2.properties. Using default config which logs errors to the console
[INFO ] 2018-xx-xx xx:xx:xx.xxx [main] scaffold - Initializing module {:module_name=>"netflow", :directory=>"/usr/share/logstash/modules/netflow/configuration"}
[INFO ] 2018-xx-xx xx:xx:xx.xxx [main] scaffold - Initializing module {:module_name=>"fb_apache", :directory=>"/usr/share/logstash/modules/fb_apache/configuration"}
[WARN ] 2018-xx-xx xx:xx:xx.xxx [LogStash::Runner] multilocal - Ignoring the 'pipelines.yml' file because modules or command line options are specified
[FATAL] 2018-xx-xx xx:xx:xx.xxx [LogStash::Runner] runner - Logstash could not be started because there is already another instance using the configured data directory.  If you wish to run multiple instances, you must change the "path.data" setting.
[ERROR] 2018-xx-xx xx:xx:xx.xxx [LogStash::Runner] Logstash - java.lang.IllegalStateException: org.jruby.exceptions.RaiseException: (SystemExit) exit
```

この場合の対処方法は、プロセスを強制的にkillします。

```bash
### Kill process
$ ps -aux | grep logstash
Warning: bad syntax, perhaps a bogus '-'? See /usr/share/doc/procps-3.2.8/FAQ
root     32061  1.7 12.8 4811812 521780 pts/0  Tl   14:12   1:06 /usr/lib/jvm/java/bin/java -Xms2g -Xmx2g -XX:+UseParNewGC -XX:+UseConcMarkSweepGC -XX:CMSInitiatingOccupancyFraction=75 -XX:+UseCMSInitiatingOccupancyOnly -XX:+DisableExplicitGC -Djava.awt.headless=true -Dfile.encoding=UTF-8 -XX:+HeapDumpOnOutOfMemoryError -cp /usr/share/logstash/logstash-core/lib/jars/animal-sniffer-annotations-1.14.jar:/usr/share/logstash/logstash-core/lib/jars/commons-compiler-3.0.8.jar:/usr/share/logstash/logstash-core/lib/jars/error_prone_annotations-2.0.18.jar:/usr/share/logstash/logstash-core/lib/jars/google-java-format-1.5.jar:/usr/share/logstash/logstash-core/lib/jars/guava-22.0.jar:/usr/share/logstash/logstash-core/lib/jars/j2objc-annotations-1.1.jar:/usr/share/logstash/logstash-core/lib/jars/jackson-annotations-2.9.1.jar:/usr/share/logstash/logstash-core/lib/jars/jackson-core-2.9.1.jar:/usr/share/logstash/logstash-core/lib/jars/jackson-databind-2.9.1.jar:/usr/share/logstash/logstash-core/lib/jars/jackson-dataformat-cbor-2.9.1.jar:/usr/share/logstash/logstash-core/lib/jars/janino-3.0.8.jar:/usr/share/logstash/logstash-core/lib/jars/javac-shaded-9-dev-r4023-3.jar:/usr/share/logstash/logstash-core/lib/jars/jruby-complete-9.1.13.0.jar:/usr/share/logstash/logstash-core/lib/jars/jsr305-1.3.9.jar:/usr/share/logstash/logstash-core/lib/jars/log4j-api-2.9.1.jar:/usr/share/logstash/logstash-core/lib/jars/log4j-core-2.9.1.jar:/usr/share/logstash/logstash-core/lib/jars/log4j-slf4j-impl-2.9.1.jar:/usr/share/logstash/logstash-core/lib/jars/logstash-core.jar:/usr/share/logstash/logstash-core/lib/jars/slf4j-api-1.7.25.jar org.logstash.Logstash -f conf.d/alb.conf
root     32231  0.0  0.0 110468  2060 pts/0    S+   15:16   0:00 grep --color=auto logstash
$ kill -9 32061
```

これでFilterについてなんとなくわかったと思います。
次は、いよいよ最終形態のInputをS3にして、OutputをElasticsearchにする構成をやっていきたいと思います。

## 最終的なパイプラインの設定ファイルが完成するよ

### Inputの編集

"Input"部分が、現状だとファイル取り込みになっているので、S3に変更します。
以下を記載します。

```bash
input {
  s3 {
    region => "ap-northeast-1"
    bucket => "bucket"
    prefix => "directory/"
    interval => "60"
    sincedb_path => "/var/lib/logstash/sincedb_alb"
  }
}
```

各オプションについて説明します。

| No. | Item         | Content                                   |
|:----|:-------------|:------------------------------------------|
| 1   | region       | AWSのリージョンを指定                     |
| 2   | bucket       | バケットを指定                            |
| 3   | prefix       | バケット配下のディレクトリを指定          |
| 4   | interval     | バケットからログを取り込む間隔を指定(sec) |
| 5   | sincedb_path | sincedbファイルの出力先を指定             |

今回は、AWSのアクセスキーとシークレットキーを指定せず、IAM Roleをインスタンスに割り当てています。
オプションで指定することも可能ですが、セキュリティ面からIAM Roleで制御してます。

### Outputの編集

やっとここまできましたね！
最後に"Output"を標準出力からElasticsearchに変更します。

```bash
output {
  elasticsearch {
    hosts => [ "localhost:9200" ]
  }
}
```

以下に各オプションについて説明します。
インデックスを任意の形で指定することも可能ですが、デフォルトのままとするため、指定はしてません。
デフォルトだと"logstash-logs-%{+YYYYMMdd}"で作成されます。

| No. | Item  | Content                   |
|:----|:------|:--------------------------|
| 1   | hosts | elasticsearchの宛先を指定 |

これで完成です！
以下に最終的なパイプラインの設定ファイルを記載します。

```bash
### Final configuration file
input {
  s3 {
    region => "ap-northeast-1"
    bucket => "bucket"
    prefix => "directory/"
    interval => "60"
    sincedb_path => "/var/lib/logstash/sincedb_alb"
  }
}
filter {
  grok {
    patterns_dir => ["/etc/logstash/patterns/alb_patterns"]
    match => { "message" => "%{ALB_ACCESS_LOG}" }
  }
  date {
    match => [ "date", "ISO8601" ]
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
```

それでは実行させるのですが、今までコマンドライン実行だったので、最後は、サービスで動かしたいと思います。

```bash
### Start logstash service
$ initctl start logstash
```

インデックスが取り込まれているかを確認します。
インデックスが日付単位で取り込まれていることがわかります。

```bash
### Index confirmation
$ curl -XGET localhost:9200/_cat/indices/logstash*
yellow open logstash-logs-2016xxxx SJ07jipISK-kDlpV5tiHiA 5 1 42 0 650.6kb 650.6kb
```

ドキュメントも確認します。
"curl -XGET localhost:9200/{index}/{type}/{id}"の形式で確認できます。
また、"?pretty"を使用することでjsonが整形されます。

```bash
$ curl -XGET 'localhost:9200/logstash-2016.08.10/doc/DTAU02EB00Bh04bZnyp1/?pretty'
{
  "_index" : "logstash-2016.08.10",
  "_type" : "doc",
  "_id" : "DTAU02EB00Bh04bZnyp1",
  "_version" : 1,
  "found" : true,
  "_source" : {
    "message" : "https 2016-08-10T23:39:43.065466Z app/my-loadbalancer/50dc6c495c0c9188  5.10.83.30:2817 10.0.0.1:80 0.086 0.048 0.037 200 200 0 57 \"GET https://www.example.com:443/ HTTP/1.1\" \"curl/7.46.0\" ECDHE-RSA-AES128-GCM-SHA256 TLSv1.2  arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067 \"Root=1-58337281-1d84f3d73c47ec4e58577259\" www.example.com arn:aws:acm:us-east-2:123456789012:certificate/12345678-1234-1234-1234-123456789012",
    "path" : [
      "/etc/logstash/alb.log",
      "/"
    ],
    "client_ip" : "5.10.83.30",
    "proto" : "https",
    "httpversion" : "1.1",
    "geoip" : {
      "postal_code" : "1091",
      "country_name" : "Netherlands",
      "city_name" : "Amsterdam",
      "ip" : "5.10.83.30",
      "location" : {
        "lon" : 4.9167,
        "lat" : 52.35
      },
      "longitude" : 4.9167,
      "region_name" : "North Holland",
      "region_code" : "NH",
      "country_code3" : "NL",
      "continent_code" : "EU",
      "timezone" : "Europe/Amsterdam",
      "latitude" : 52.35,
      "country_code2" : "NL"
    },
    "@version" : "1",
    "response_processing_time" : 0.037,
    "backend_port" : 80,
    "target_status_code" : 200,
    "user_agent" : "curl/7.46.0",
    "sent_bytes" : 57,
    "ssl_protocol" : "TLSv1.2",
    "client_port" : 2817,
    "date" : "2016-08-10T23:39:43.065466Z",
    "port" : "443",
    "target_processing_time" : 0.048,
    "elb_status_code" : "200",
    "request_processing_time" : 0.086,
    "backend_ip" : "10.0.0.1",
    "urihost" : "www.example.com:443",
    "ssl_cipher" : "ECDHE-RSA-AES128-GCM-SHA256",
    "target_group_arn" : "arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067",
    "host" : "ip-172-31-50-36",
    "trace_id" : "Root=1-58337281-1d84f3d73c47ec4e58577259",
    "@timestamp" : "2016-08-10T23:39:43.065Z",
    "verb" : "GET",
    "class" : "https",
    "request" : "https://www.example.com:443/",
    "elb" : "app/my-loadbalancer/50dc6c495c0c9188",
    "received_bytes" : 0
  }
}
```

Elasticsearchに取り込まれたことが確認できました。
次は、LogstashのイケてるMultiple Pipelinesについて触れていきたいと思います。


