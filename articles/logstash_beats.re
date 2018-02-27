
= はじまるよ


みなさんログは好きですか？@<br>{}
今までたくさんのログを見てこられた方は多くいるのではないでしょうか。@<br>{}
そして、多くの苦労を伴ってきたのではないでしょうか。@<br>{}
これからエンジニアとなり、ログと向き合うことになる方もいるかと思います。@<br>{}
そんな方々へ、少しでもログの楽しさが伝われば嬉しいなと思ってます。  


== こんな流れで話すよ


それでは、ここから具体的にどんな内容について書いてあるのかざっくり書くと、ログの取込方法や、多種多様なログフォーマットに合わせたフィルターの書き方などについてです。

 1. 使用するプロダクトや環境について
 1. Logstashを使ってみる
 1. Beatsという便利なデータシッパーを使ってみる
 1. まとめ


== 使用するプロダクトと実行環境について


Elasticsearch社が提供するElastic Stackを利用します。@<br>{}
以下のプロダクトを利用してログの世界に触れていきたいと思います。

 * Elasticsearch 6.2.2
 * Logstash 6.2.2
 * Kibana 6.2.2
 * Metricbeat 6.2.2
 * Auditbeat 6.2.2
 * Packetbeat 6.2.2


//quote{
Download Elastic Stack@<br>{}
https://www.elastic.co/jp/products

//}


簡単にこれらのプロダクトについて紹介します。


=== Elasticsearch


Elasticsaerchは、オープンソースの分散型RESTfull検索/分析エンジンです。@<br>{}
今回、Logstashのストア先として利用します。


=== Logstash


Logstashは、オープンソースログ収集管理ツールです。@<br>{}
取り込み先としては、検索エンジンのElasticsearchと組み合わせることでニアリアルタイムにログを取り込むことができます。


=== kibana


Kibanaは、オープンソースのビジュアライズツールです。@<br>{}
Elasticserchのデータをビジュアライズします。


=== Beats


Beatsは、様々な用途に合わせてデータを簡単に送ることができるオープンソースの軽量データシッパーです。@<br>{}
Beatsファミリーとして、以下があります。

 * Filebeat
 * Metricbeat
 * Packetbeat
 * Winlogbeat
 * Auditbeat
 * Heartbeat


=== Elastic Stackを動かす実行環境


* Amazon Linux AMI 2017.09.1 (HVM), SSD Volume Type - ami-97785bed
* t2.medium
  - vCPU: 2
  - Mem: 4


= Logstashを使ってみる


Logstashは、"Input""Filter""Output"の構成で記述します。@<br>{}
Inputでログのソースを指定し、Filterで必要な要素に分解します。最後のOutputで出力先を指定します。@<br>{}
今回は、AWS環境なので、Inputは、S3からログを取得して、OutputでElasticsearchにストアさせます。  



それでは、ここから実際にLogstashをインストールして、どのようなことを行うかを見ていきたいと思います。  


== 実行環境を準備するよ


Logstashのインストールは以下の公式でも記載されてるのですが、英語です。@<br>{}
やはり抵抗感を覚える人もいると思うので、できる限りわかりやすく日本語で書きます。@<br>{}
正直、英語ぐらいわかるわー！って人は、飛ばしちゃってください。  


//quote{
Install Logstash: https://www.elastic.co/guide/en/logstash/current/installing-logstash.html

//}

=== Java 8インストール


Logstashをインストールするにあたり、Java 8が必要なので、インストールします。
Javaの切り替えにalternativesコマンドを使用して変更します。


//emlist[][bash]{
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
//}

=== GPGキーをインポート

//emlist[][bash]{
### Import GPG-Key
$ rpm --import https://artifacts.elastic.co/GPG-KEY-elasticsearch
//}

=== YUMリポジトリの追加


logstashのパッケージを取得するため、YUMリポジトリを追加します。@<br>{}
"/etc/yum.repo/"配下に"elasticstack.repo"というファイルを作成します。@<br>{}
公式では、logstash.repoとなっておりますが、今回はElasticsearchなどもインストールするため、汎用的な名前にしました。要は任意で問題ないということです。


//emlist[][bash]{
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
//}

=== Logstashをインストール


最後にLogstashをインストールします。


//emlist[][bash]{
### Install Logstash
$ sudo yum install logstash
$ /usr/share/logstash/bin/logstash --version
logstash 6.2.2
//}


S3をデータソースにするため、"S3 Input Plugin"をインストールします。


//quote{
S3 Input Plugin: https://www.elastic.co/guide/en/logstash/current/plugins-inputs-s3.html

//}

//emlist[][bash]{
$ /usr/share/logstash/bin/logstash-plugin install logstash-input-s3
Validating logstash-input-s3
Installing logstash-input-s3
Installation successful
//}

=== Elasticsearchをインストール


Output先としてElasticsearchを利用するため、Elasticsearchをインストールします。


//emlist[][bash]{
### Install Elasticsearch
$ sudo yum install elasticsearch
$ /usr/share/elasticsearch/bin/elasticsearch --version
Version: 6.2.2, Build: 10b1edd/2018-02-16T19:01:30.685723Z, JVM: 1.8.0_161
//}


サービスの自動起動をする場合は、以下を実行してください。


//emlist[][bash]{
### Auto start setting
$ sudo chkconfig --add elasticsearch
$ chkconfig --list | grep elasticsearch
elasticsearch   0:off   1:off   2:on    3:on    4:on    5:on    6:off
//}

== Elasticsearchの準備するよ


ここからはElasticsearchの設定ファイルの編集をします。


=== Elasticsearchのディレクトリ構成について


Elasticsearchのディレクトリ構成は以下です。@<br>{}
"elasticsearch.yml"と"jvm.options"の設定を編集します。
ログの出力部分など編集したい場合は、"log4j2.properties"を編集してください。


//emlist[][bash]{
### Elasticsearch directory structure
/etc/elasticsearch/
 ┣ elasticsearch.yml
 ┣ jvm.options
 ┗ log4j2.properties
//}

=== jvm.optionsの編集


"jvm.options"からヒープサイズを変更することが可能で、"Xms(minimum heap size)"と"Xmx(maximum heap size) "を変更します。@<br>{}
Elasticsearch社の公式に書かれている設定アドバイスは以下です。

 * 最小ヒープサイズ(Xms)と最大ヒープサイズ(Xmx)の値を等しくする
 * ヒープサイズを上げすぎるとGCの休止をまねく可能性がある
 * Xmxは、物理メモリの50%を割り当てて、ファイルシステム側にも十分に残すようにする
 * 割り当てるメモリは、32GB以下にする



上記を踏まえて"jvm.options"の設定をします。
インスタンスのメモリは、4GBなので物理メモリの50%の2GBを割り当てます。


//emlist[][bash]{
### Heap size change
$ vim /etc/elasticsearch/jvm.options
-Xms2g
-Xmx2g
//}

//quote{
Settings the heap size:
https://www.elastic.co/guide/en/elasticsearch/reference/current/heap-size.html

//}

=== elasticserch.ymlの編集


"elasticsearch.yml"でノード名や、クラスタ名の編集が可能ですが、デフォルト値とします。@<br>{}
今回は、どこからでもアクセス可能とするため、"network.host"のみを編集します。
"0.0.0.0"と設定することで、どこからでもアクセス可能となります。


//emlist[][bash]{
### Settings to enable access from anywhere
$ network.host: 0.0.0.0
//}


参考までに実運用では必要となりうる"elasticserch.yml"の設定値について記載します。

//table[tbl1][]{
No.	Item	Content
-----------------
1	cluster.name: my-application	クラスタ名の指定
2	node.name	ノード名の指定
3	network.host	アクセス元のネットワークアドレスを指定することで制限をかけることが可能
4	discovery.zen.ping.unicast.hosts	クラスタを組む際にノードを指定
5	discovery.zen.minimum@<b>{master}nodes	必要最低限のノード数を指定
//}


Elasticsearchを起動し、動作確認をします。
curlで実行し、レスポンスが返ってくれば問題なく起動してます。


//emlist[][bash]{
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
    "build_date" : "2018-xx-xxTxx:xx:xx.xxxxxZ",
    "build_snapshot" : false,
    "lucene_version" : "7.2.1",
    "minimum_wire_compatibility_version" : "5.6.0",
    "minimum_index_compatibility_version" : "5.0.0"
  },
  "tagline" : "You Know, for Search"
}
//}


Elasticsearchの準備が完了です。


== Logstashの設定ファイルを触ってみる


Logstashのディレクトリ構成は以下です。


//emlist[][bash]{
### Elasticsearch directory structure
/etc/logstash/
 ┣ conf.d
 ┣ jvm.options
 ┣ log4j2.properties
 ┣ logstash.yml
 ┣ pipelines.yml
 ┗ startup.options
//}


各ファイルやディレクトリにについて説明します。

//table[tbl2][]{
No.	Item	Content
-----------------
1	conf.d	Input/Filter/Outputのパイプラインを記載したファイルの格納場所
2	jvm.options	jvmオプションの管理ファイル
3	log4j2.properties	log4jのプロパティ管理ファイル
4	logstash.yml	Logstashの設定ファイル
5	pipelines.yml	パイプラインを複数実行する際に定義するファイル
6	startup.options	Logstash起動時に利用されるファイル
//}

=== logstash.ymlの編集


今回は、"logstash.yml"の編集は行いませんが、このファイルでどんなことができるかをサクッと書いておきます。@<br>{}
logstash.ymlでは、パイプラインのバッチサイズやディレイ設定が可能です。@<br>{}
例えば、以下のように階層やフラットな構造で記載します。


//emlist[][bash]{
### hierarchical form
pipeline:
  batch:
    size: 125
    delay: 50
### flat key
pipeline.batch.size: 125
pipeline.batch.delay: 50
//}


また、パイプラインを実行するWoker数を変更することも可能です。
変更する際は、"pipeline.workers"の数を変更します。
Woker数の目安は、割り当てたいCPUコア数とイコールにするのが良いです。


//emlist[][bash]{
### Change Woker
pipeline.workers: 2
//}


公式に詳細が記載されているので参考にすると幸せになれます。


//quote{
Settings File:
https://www.elastic.co/guide/en/logstash/current/logstash-settings-file.html

//}

=== パイプラインを実行してみる


ここからLogstashのパイプラインを動かしていきたいと思います。@<br>{}
まずは、単純にコマンドラインからLogstashを実行したいと思います。



パイプラインの設定ファイルを作成します。@<br>{}
このパイプラインは、単純に標準入力から標準出力するものです。


//emlist[][bash]{
### Cleate pipeline
$ vim /etc/logstash/conf.d/test.conf
input { 
  stdin { }
}
output {
  stdout { codec => rubydebug }
}
//}


以下のコマンドを実行し、"Pipelines running"と表示されたら任意の文字を標準入力します。
入力した文字（ここではtest）が"message"に表示にされていることがわかります。


//emlist[][bash]{
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
//}

=== さらにパイプラインを実行してみる


ここからは実際のログを利用してパイプラインを扱っていきたいと思います。@<br>{}
対象のログとして、AWSのALBログを利用します。  



AWS公式ページに記載されているサンプルログを利用します。@<br>{}
アクセス元のIPアドレスは、Elasticsearch社の公式にあるグローバルIPに置き換えてます(5.10.83.30)@<br>{}
理由は、GeoIPフィルタを利用するためです。


//quote{
Access Logs for Your Application Load Balancer: 
https://docs.aws.amazon.com/elasticloadbalancing/latest/application/load-balancer-access-logs.html

//}

//emlist{
https 2016-08-10T23:39:43.065466Z app/my-loadbalancer/50dc6c495c0c9188 
5.10.83.30:2817 10.0.0.1:80 0.086 0.048 0.037 200 200 0 57 
"GET https://www.example.com:443/ HTTP/1.1" "curl/7.46.0" ECDHE-RSA-AES128-GCM-SHA256 TLSv1.2 
arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067
"Root=1-58337281-1d84f3d73c47ec4e58577259" www.example.com arn:aws:acm:us-east-2:123456789012:certificate/12345678-1234-1234-1234-123456789012
//}


このサンプルログを"/etc/logstash/"配下に配置します。@<br>{}
ログのファイル名を"alb.log"にします。


//emlist[][bash]{
$ vim /etc/logstash/alb.log
https 2016-08-10T23:39:43.065466Z app/my-loadbalancer/50dc6c495c0c9188  5.10.83.30:2817 10.0.0.1:80 0.086 0.048 0.037 200 200 0 57 "GET https://www.example.com:443/ HTTP/1.1" "curl/7.46.0" ECDHE-RSA-AES128-GCM-SHA256 TLSv1.2  arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067 "Root=1-58337281-1d84f3d73c47ec4e58577259" www.example.com arn:aws:acm:us-east-2:123456789012:certificate/12345678-1234-1234-1234-123456789012
//}


パイプラインの設定ファイルを作成します。@<br>{}
先程までは、"Input"を標準入力としてましたが、ファイルを取り込むので"File input plugin"を使用します。@<br>{}
標準でインストールされているので、インストールは不要です。  



それでは、新しく"alb.conf"という設定ファイルを作成します。


//emlist[][bash]{
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
//}


追記した部分を以下で説明します。

//table[tbl3][]{
No.	Item	Content
-----------------
1	path	取り込むファイルを指定します(ディレクトリ指定の"*"指定も可能)
2	start_position	Logstash起動した時にどこから読み込むかの指定(デフォルトは、end)
3	sincedb_path	ログファイルを前回どこまで取り込んだかを記載するファイル
//}


ではでは、作った設定ファイルで実行します。


//emlist[][bash]{
### Run Pipeline
$ /usr/share/logstash/bin/logstash -f /etc/logstash/conf.d/alb.conf
{
    "@timestamp" => 2018-xx-xxTxx:xx:xx.xxxZ,
          "path" => "/etc/logstash/alb.logs",
       "message" => "https 2016-08-10T23:39:43.065466Z app/my-loadbalancer/50dc6c495c0c9188  5.10.83.30:2817 10.0.0.1:80 0.086 0.048 0.037 200 200 0 57 "GET https://www.example.com:443/ HTTP/1.1" "curl/7.46.0" ECDHE-RSA-AES128-GCM-SHA256 TLSv1.2  arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067 "Root=1-58337281-1d84f3d73c47ec4e58577259" www.example.com arn:aws:acm:us-east-2:123456789012:certificate/12345678-1234-1234-1234-123456789012,
      "@version" => "1",
          "host" => "ip-172-31-50-36"
}
//}


標準入力で実行した時と同様に"message"に取り込んだログが出力されていることがわかります。@<br>{}
ただ、これでは構造化した形でElasticsearchにストアされないため、検索性が損なわれます。@<br>{}
その解決方法として"Filter"を利用します。


=== Filterを使ってみる


"Filter"では何を行うかというと、取得したログを正規表現でパースするGrokフィルタや、地理情報を得るためのGioIPフィルタを施すことができます。@<br>{}
今回のALBもGrokフィルタなどを使うことで構造化することが可能です。  



まず、ALBのログフォーマットを把握する必要があります。@<br>{}
以下に記載します。


//emlist[][bash]{
type timestamp elb client:port target:port request_processing_time target_processing_time response_processing_time elb_status_code target_status_code received_bytes sent_bytes "request" "user_agent" ssl_cipher ssl_protocol target_group_arn trace_id domain_name chosen_cert_arn
//}


各フィールドについて以下に記載します。  

//table[tbl4][]{
Log	Field	Type
-----------------
type	class	string
timestamp	date	date
elb	elb	string
client_ip	client_ip	int
client_port	target_port	int
target_ip	target_ip	int
target_port	target_port	int
request@<b>{processing}time	request@<b>{processing}time	float
target@<b>{processing}time	target@<b>{processing}time	float
response@<b>{processing}time	response@<b>{processing}time	float
elb@<b>{status}code	elb@<b>{status}code	string
target@<b>{status}code	target@<b>{status}code	string
received_bytes	received_bytes	int
sent_bytes	sent_bytes	int
request	ELB@<b>{REQUEST}LINE	string
user_agent	user_agent	string
ssl_cipher	ssl_cipher	string
ssl_protocol	ssl_protocol	string
target@<b>{group}arn	target@<b>{group}arn	string
trace_id	trace_id	string
//}


このフィールド単位でフィルタをかけられるようkey-valueにGrokフィルタで分解します。@<br>{}
ALB用の"grok-patterns"を記載したパターンファイルを作成します。  



が、その前にパターンファイルを格納するディレクトリを作成します。@<br>{}
パイプラインの設定ファイルにGrokフィルタを記載するでもいいのですが、可読性を上げるため外だしにしてます。


//emlist[][bash]{
### Create directory
$ mkdir /etc/logstash/patterns
$ ll | grep patterns
drwxr-xr-x 2 root root 4096 xxx xx xx:xx patterns
//}


patternsディレクトリ配下にALBのパターンファイルを作成します。@<br>{}
中身については、闇深いのでここでは説明しません。。無邪気に貼っつけてください。@<br>{}
また、Typeは、インデックステンプレートで作成するのが一般的かと思いますが、今回は、パターンファイルの中で指定します。@<br>{}
(実はここで指定できるんだよってことも知ってもらうという)


//emlist[][bash]{
$ sudo vim /etc/logstash/patterns/alb_patterns
# Application Load Balancing
ALB_ACCESS_LOG %{NOTSPACE:class} %{TIMESTAMP_ISO8601:date} %{NOTSPACE:elb}  (?:%{IP:client_ip}:%{INT:client_port:int}) (?:%{IP:backend_ip}:%{INT:backend_port:int}|-) (:?%{NUMBER:request_processing_time:float}|-1) (?:%{NUMBER:target_processing_time:float}|-1) (?:%{NUMBER:response_processing_time:float}|-1) (?:%{INT:elb_status_code}|-) (?:%{INT:target_status_code:int}|-) %{INT:received_bytes:int} %{INT:sent_bytes:int} \"%{ELB_REQUEST_LINE}\" \"(?:%{DATA:user_agent}|-)\" (?:%{NOTSPACE:ssl_cipher}|-) (?:%{NOTSPACE:ssl_protocol}|-)  %{NOTSPACE:target_group_arn} \"%{NOTSPACE:trace_id}\"
//}


alb.confに"Fiter"を追加します。


//emlist[][bash]{
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
//}


更新できたら実行します。@<br>{}
いい感じにkey-valueのかたちになっていることがわかります。


//emlist[][bash]{
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
//}


それでは、Filterで記載している内容について説明します。  



正規表現でパースする際にgrokフィルタを使用します。@<br>{}
"patterns_dir"で外だししているパターンファイルを呼び出すことができます。@<br>{\}
"match"で"message"に取り込まれている値を対象にGrok-Patterns(ここでいうALB}ACCESS_LOG)を適用しています。


//emlist[][bash]{
### grok-filter
  grok {
    patterns_dir => ["/etc/logstash/patterns/alb_patterns"]
    match => { "message" => "%{ALB_ACCESS_LOG}" }
  }
//}


dateフィルタで"@timestamp"をgrokフィルタで抽出した"date" で置き換えます。@<br>{}
また、タイムゾーンを"Asia/Tokyo"に指定してます。


//emlist[][bash]{
  date {
    match => [ "date", "ISO8601" ]
    timezone => "Asia/Tokyo"
    target => "@timestamp"
  }
//}


geoipフォルタを使用することでIPアドレスから地理情報を取得することが可能です。@<br>{}
geoipフィルタの対象とするため、"client@<b>{ip"を指定してます。@<br>{\}
"client}ip"を指定する意図は、どこの国からアクセスがきているかを把握するためです。


//emlist[][bash]{
  geoip {
    source => "client_ip"
  }
//}


今回は使用していないですが、不要な値は、mutateで削除可能です。@<br>{}
例えば、messageの値は、全てkey-valueでストアされているから不要なので削除といったことも可能です。@<br>{}
個人的には、ストアされたデータで"_grokparsefailure"が発生した時の場合も踏まえると、残した方がいいと思ってます。@<fn>{1}  



messageを削除する場合は、Filterにmutateを追加します。


//emlist[][bash]{
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
//}


補足ですが、コマンドラインで実行している際に以下のようなエラーが発生した場合は、Logstashのプロセスがすでに立ち上がっている時に発生します。


//emlist[][bash]{
### Error executing logstash
$ /usr/share/logstash/bin/logstash -f conf.d/alb.conf
WARNING: Could not find logstash.yml which is typically located in $LS_HOME/config or /etc/logstash. You can specify the path using --path.settings. Continuing using the defaults
Could not find log4j2 configuration at path /usr/share/logstash/config/log4j2.properties. Using default config which logs errors to the console
[INFO ] 2018-xx-xx xx:xx:xx.xxx [main] scaffold - Initializing module {:module_name=>"netflow", :directory=>"/usr/share/logstash/modules/netflow/configuration"}
[INFO ] 2018-xx-xx xx:xx:xx.xxx [main] scaffold - Initializing module {:module_name=>"fb_apache", :directory=>"/usr/share/logstash/modules/fb_apache/configuration"}
[WARN ] 2018-xx-xx xx:xx:xx.xxx [LogStash::Runner] multilocal - Ignoring the 'pipelines.yml' file because modules or command line options are specified
[FATAL] 2018-xx-xx xx:xx:xx.xxx [LogStash::Runner] runner - Logstash could not be started because there is already another instance using the configured data directory.  If you wish to run multiple instances, you must change the "path.data" setting.
[ERROR] 2018-xx-xx xx:xx:xx.xxx [LogStash::Runner] Logstash - java.lang.IllegalStateException: org.jruby.exceptions.RaiseException: (SystemExit) exit
//}


この場合の対処方法は、プロセスを強制的にkillします。


//emlist[][bash]{
### Kill process
$ ps -aux | grep logstash
Warning: bad syntax, perhaps a bogus '-'? See /usr/share/doc/procps-3.2.8/FAQ
root     32061  1.7 12.8 4811812 521780 pts/0  Tl   14:12   1:06 /usr/lib/jvm/java/bin/java -Xms2g -Xmx2g -XX:+UseParNewGC -XX:+UseConcMarkSweepGC -XX:CMSInitiatingOccupancyFraction=75 -XX:+UseCMSInitiatingOccupancyOnly -XX:+DisableExplicitGC -Djava.awt.headless=true -Dfile.encoding=UTF-8 -XX:+HeapDumpOnOutOfMemoryError -cp /usr/share/logstash/logstash-core/lib/jars/animal-sniffer-annotations-1.14.jar:/usr/share/logstash/logstash-core/lib/jars/commons-compiler-3.0.8.jar:/usr/share/logstash/logstash-core/lib/jars/error_prone_annotations-2.0.18.jar:/usr/share/logstash/logstash-core/lib/jars/google-java-format-1.5.jar:/usr/share/logstash/logstash-core/lib/jars/guava-22.0.jar:/usr/share/logstash/logstash-core/lib/jars/j2objc-annotations-1.1.jar:/usr/share/logstash/logstash-core/lib/jars/jackson-annotations-2.9.1.jar:/usr/share/logstash/logstash-core/lib/jars/jackson-core-2.9.1.jar:/usr/share/logstash/logstash-core/lib/jars/jackson-databind-2.9.1.jar:/usr/share/logstash/logstash-core/lib/jars/jackson-dataformat-cbor-2.9.1.jar:/usr/share/logstash/logstash-core/lib/jars/janino-3.0.8.jar:/usr/share/logstash/logstash-core/lib/jars/javac-shaded-9-dev-r4023-3.jar:/usr/share/logstash/logstash-core/lib/jars/jruby-complete-9.1.13.0.jar:/usr/share/logstash/logstash-core/lib/jars/jsr305-1.3.9.jar:/usr/share/logstash/logstash-core/lib/jars/log4j-api-2.9.1.jar:/usr/share/logstash/logstash-core/lib/jars/log4j-core-2.9.1.jar:/usr/share/logstash/logstash-core/lib/jars/log4j-slf4j-impl-2.9.1.jar:/usr/share/logstash/logstash-core/lib/jars/logstash-core.jar:/usr/share/logstash/logstash-core/lib/jars/slf4j-api-1.7.25.jar org.logstash.Logstash -f conf.d/alb.conf
root     32231  0.0  0.0 110468  2060 pts/0    S+   15:16   0:00 grep --color=auto logstash
$ kill -9 32061
//}


Filterについてなんとなくわかったと思います。@<br>{}
次は、いよいよ最終形態のInputをS3にして、OutputをElasticsearchにする構成をやっていきたいと思います。


== 最終的なパイプラインの設定ファイルが完成するよ

=== Inputの編集


"Input"部分が、現状だとファイル取り込みになっているので、S3に変更します。@<br>{}
以下を記載します。


//emlist[][bash]{
input {
  s3 {
    region => "ap-northeast-1"
    bucket => "bucket"
    prefix => "directory/"
    interval => "60"
    sincedb_path => "/var/lib/logstash/sincedb_alb"
  }
}
//}


各オプションについて説明します。

//table[tbl5][]{
No.	Item	Content
-----------------
1	region	AWSのリージョンを指定
2	bucket	バケットを指定
3	prefix	バケット配下のディレクトリを指定
4	interval	バケットからログを取り込む間隔を指定(sec)
5	sincedb_path	sincedbファイルの出力先を指定
//}


今回は、AWSのアクセスキーとシークレットキーを指定せず、IAM Roleをインスタンスに割り当てています。@<br>{}
オプションで指定することも可能ですが、セキュリティ面からIAM Roleで制御してます。@<br>{}
IAM Roleの設定方法については、AWS公式ページを参照ください。


=== Outputの編集


やっとここまできましたね！@<br>{}
最後に"Output"を標準出力からElasticsearchに変更します。


//emlist[][bash]{
output {
  elasticsearch {
    hosts => [ "localhost:9200" ]
  }
}
//}


以下に各オプションについて説明します。@<br>{}
インデックスを任意の形で指定することも可能ですが、デフォルトのままとするため、指定はしてません。@<br>{}
デフォルトだと"logstash-logs-%{+YYYYMMdd}"で作成されます。

//table[tbl6][]{
No.	Item	Content
-----------------
1	hosts	elasticsearchの宛先を指定
//}


これで完成です！
以下に最終的なパイプラインの設定ファイルを記載します。


//emlist[][bash]{
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
//}


それでは実行させるのですが、今まではコマンドライン実行だったので、最後は、サービスで動かしたいと思います。


//emlist[][bash]{
### Start logstash service
$ initctl start logstash
//}


インデックスが取り込まれているかを確認します。@<br>{}
インデックスが日付単位で取り込まれていることがわかります。


//emlist[][bash]{
### Index confirmation
$ curl -XGET localhost:9200/_cat/indices/logstash*
yellow open logstash-logs-2016xxxx SJ07jipISK-kDlpV5tiHiA 5 1 42 0 650.6kb 650.6kb
//}


ドキュメントも確認します。
"curl -XGET localhost:9200/{index}/{type}/{id}"の形式で確認できます。
また、"?pretty"を使用することでjsonが整形されます。


//emlist[][bash]{
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
//}


Elasticsearchに取り込まれたことが確認できました。
次は、LogstashのイケてるMultiple Pipelinesについて触れていきたいと思います。


= Multiple Pipelinesの世界へようこそ

== 複数のログを取り扱う場合


ここまで一つのログを対象にしてきましたが、実際は多数のログを対象にすることが多いと思います。@<br>{}
多数のログを取り扱う場合は、conf.d配下にパイプラインの設定ファイルを配置します。@<br>{}
例えば、ALBのログとhttpdログの場合は、以下になります。


//emlist[][bash]{
### Logstash directory
/etc/logstash/
 ┣ conf.d
 ┃ ┣ alb.conf
 ┃ ┗ httpd.conf
 ┣ jvm.options
 ┣ log4j2.properties
 ┣ logstash.yml
 ┣ pipelines.yml
 ┗ startup.options
//}


このようにconf.dに配置したファイルが対象の設定ファイルと見なされます。@<br>{}
ログによっては、ログ量やフィルタなど負荷のかかり具合が違ってくるのですが、柔軟なリソース供給ができません。@<br>{}
前章で説明したWoker数などを設定ファイル毎に設定することができないのです。  



また、一つの設定ファイルにまとめて設定ファイルを作成するケースもあると思います。@<br>{}
その場合は、以下のようにif文を駆使して対応するかたちになります。


//emlist[][bash]{
### Sample pipeline_file
input {
  s3 {
    tags => "alb"
    bucket => "hoge"
    region => "ap-northeast-1"
    prefix => "hoge/"
    interval => "60"
    sincedb_path => "/var/lib/logstash/sincedb_alb"
  }
  file {
    path => "/etc/logstash/log/httpd_access.log"
    tags => "httpd"
    start_position => "beginning"
    sincedb_path => "/var/lib/logstash/sincedb_httpd"
  }
}
filter {
  if "alb" in [tags] {
    grok {
      patterns_dir => ["/etc/logstash/patterns/cloudfront_patterns"]
      match => { "message" => "%{ALB_ACCESS}" }
      add_field => { "date" => "%{date01} %{time}" }
    }
    date {
      match => [ "date", "yy-MM-dd HH:mm:ss" ]
      locale => "en"
      target => "@timestamp"
    }
    geoip {
      source => "c_ip"
    }
    useragent {
      source => "User_Agent"
      target => "useragent"
    }
    mutate {
      convert => [ "time_taken", "float" ]
      remove_field => [ "message" ]
    }
  else if "httpd" in [tags] {
    grok {
      patterns_dir => ["/etc/logstash/patterns/httpd_patterns"]
      match => { "message" => "%{HTTPD_COMMON_LOG}" }
    }
    geoip {
      source => "clientip"
    }
    date {
      match => [ "date", "dd/MMM/YYYY:HH:mm:ss Z" ]
      locale => "en"
      target => "timestamp"
    }
    mutate {
      remove_field => [ "message", "path", "host", "date" ]
    }
  }
}
output {
  if "alb" in [tags] {
    elasticsearch {
      hosts => [ "localhost:9200" ]
      index => "elb-logs-%{+YYYYMMdd}"
    }
  }
  else if "httpd" in [tags] {
    elasticsearch {
      hosts => [ "localhost:9200" ]
      index => "httpd-logs-%{+YYYYMMdd}"
    }
  }
}
//}


このように設定ファイルやリソースを柔軟に設定ができなかったのですが、Logstashの6系から可能になりました。@<br>{}
それでは、Multiple Pipelinesの説明をしていきたいと思います。


== Multiple Pipelines


Multiple Pipelinesを使用するには、"pipelines.yml"に対象のパイプラインの設定ファイルを指定します。@<br>{}
例えば、ALBを対象とした設定ファイルとHTTPDを対象とした設定ファイルがあったとします。@<br>{}
その際の記載は以下になります。


//emlist[][bash]{
### pipelines.yml
vim /etc/logstash/pipelines.yml
- pipeline.id: alb
  pipeline.batch.size: 125
  path.config: "/etc/logstash/conf.d/alb.cfg"
  pipeline.workers: 1
- pipeline.id: httpd
  pipeline.batch.size: 125
  path.config: "/etc/logstash/conf.d/httpd.cfg"
  pipeline.workers: 1
//}


前回の章で説明したLogstashのオプションを個別で設定することができます。@<br>{}
そのため、"pipeline.workers"で割り当てるWorker数を指定することが可能です。  



logstash.ymlでは、"path.config"の指定を以下のように一律で行なっていました。


//quote{
path.config: /etc/logstash/conf.d/*.conf

//}


pipelines.ymlでは、以下のように個別に指定します。
拡張子は、"cfg"とします。


//quote{
path.config: /etc/logstash/conf.d/*.cfg

//}


"pipeline.id"は、任意の名前で記載します。



これでMulti Pipelinesの設定は完了です。
あとは、conf.d配下のファイル名をcfgの拡張子に変更するだけです。



実際にALBの設定ファイルを用いて起動までやりたいと思います。



pipelines.ymlを作成します。


//emlist[][bash]{
### create pipelines.yml
vim /etc/logstash/pipelines.yml
- pipeline.id: alb
  pipeline.batch.size: 125
  path.config: "/etc/logstash/conf.d/alb.cfg"
  pipeline.workers: 1
//}


"alb.conf"のファイル名を"alb.cfg"に変更します。


//emlist[][bash]{
### Change extension
mv /etc/logstash/conf.d/alb.conf /etc/logstash/conf.d/alb.cfg 
//}


それでは起動します。


//emlist[][bash]{
### Start logstash service
$ initctl start logstash
//}


起動時にエラーが発生していないかをログで確認します。
"ERROR"が発生していないかを確認し、"Pipelines running"であることを確認します。


//emlist[][bash]{
###
$ tail -f /var/log/logstash/logstash-plain.log
[2018-xx-xxTxx:xx:xx,xxx][INFO ][logstash.agent           ] Pipelines running {:count=>1, :pipelines=>["alb"]}
//}


これからは、Multiple Pipelinsを使いましょう！


= Beats


Beatsは、シンプルなデータ取り込みツールです。@<br>{}
あれ？Logstashは？と思う方もいると思いますが、Logstashは、豊富な機能を持ってます。@<br>{}
前回の章で説明したGrokフィルダで複雑なログを取り込むことも可能ですし、"Input"でデータソースを多種多様に選択することが可能です。
そのため、Logstashを利用するには、学習コストもそれなりに発生するので、手軽に利用することができません。  



そこで、手軽にデータを取り込みたい時に利用するのがBeatsです。@<br>{}
何が手軽かというと設定が、YAMLで完結するのです。@<br>{}
しかもほぼ設定する箇所はないです。



この章ではBeatsに少しでも体験して頂ければと思います。


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


Filebeatを使用することで、Apache、Nginx、MySQLなどのログ収集、パースが簡単にできます。@<br>{}
また、KibanaのDashboardも生成するため、すぐにモニタリングを始めることができます。


=== Filebeatをインストール


Filebeatをインストールします。


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


FilebeatのNginx Moduleを使用して、どれだけ楽に構築できるかを触れたいと思います。@<br>{}
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


Kibanaへのアクセス元の制限をしないため、"server.host"の設定を変更します。@<fn>{2}


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


curlを実行し、アクセスログが出力されているかを確認します。@<br>{}
また、ステータスコード200が返ってきていることを確認します。


//emlist[][bash]{
### Check access.log
$ tail -f /var/log/nginx/access.log
127.0.0.1 - - [xx/xxx/2018:xx:xx:xx +0000] "GET / HTTP/1.1" 200 3770 "-" "curl/7.53.1" "-"
//}

=== Filebeat Module


Filebeatの設定ファイルを編集する前に、"filebeat.yml"のファイル置き換えとファイル名変更を行います。@<br>{}
理由は、"filebeat.reference.yml"にすべてのModuleなどが記載されているため、簡易的に利用できるためです。


//emlist[][bash]{
### Change file name
mv /etc/filebeat/filebeat..yml /etc/filebeat/filebeat.yml_origin
mv /etc/filebeat/filebeat.reference.yml /etc/filebeat/filebeat.yml
//}


"filebeat.yml"の編集を行い、Nginxの有効化、"Output"をElasticsearchに設定を行います。@<br>{}
また、起動時にKibanaのDashboardを作成するよう設定します。   



"filebeat.yml"でNginxのModuleを有効化します。@<br>{}
ログのパスはデフォルトから変更してなければ、変更不要です。@<br>{}
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


"filebeat.reference.yml"をベースに作成しているため、デフォルトでkafkaが"enabled: true"になっています。@<br>{}
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


以下のトップページが開きます。@<br>{}
左ペインにある"Management"をクリックします。  



[filebeat01.png]



"Index Patterns"をクリックします。



[filebeat02.png]



Filebeatのインデックスパターンが登録されていることがわかります。



[filebeat03.png]



左ペインにある"Dashboard"をクリックします。@<br>{}
様々なDashboardが登録されていることがわかります。@<br>{}
Logstashなどでログを取り込んだ場合は、Dashboardを一から作成する必要がありますが、Beatsの場合は、あらかじめ用意されてます。



[filebeat04.png]



今回は、Nginxの"[Filebeat Nginx] Overview"というDashboardをクリックします。@<br>{}
取り込んだログがDashboardに表示されていることがわかります。



[filebeat05.png]



いかがでしたか？@<br>{}
他にも取り込みたいログがあれば、"filebeat.yml"のModuleを有効化するだけで簡単にモニタリングができるようになります。  



次は、サーバリソースのモニタリングする"Metricbeat"についてです。


== Metricbeat


Metricbeatは、サーバのリソース(CPU/Mem/process..etc)を簡単にモニタリングすることができます。@<br>{}
その他にもDockerやElasticsaerchなども対応しており、様々なプロダクトをモニタリングが可能です。  



また、先ほどのFilebeatと同様にYAMLを編集するだけなので、学習コストもほぼいらずに導入できます。@<br>{}
今回は、サーバのメトリックをモニタリングできるところまで見たいと思います。  



それでは、早速インストールしていきます。


//emlist[][bash]{
### Install Metricbeat
$ yum install metricbeat
//}


MetricbeatもFilebeat同様にベースの設定ファイル(metricbeat.reference.yml)があるのですが、デフォルト有効化されているModuleが多いため、以下の設定ファイルを使用します。@<br>{}
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


Filebeatと同様にデータが取り込まれているかをKibanaを開いて確認します。@<br>{}
ブラウザを開いてKibanaへアクセスします。


//quote{
http://{Global_IP}:5601

//}


"Index Patterns"の画面を開くとFilebeatのインデックスパターンの他にMetricbeatのインデックスパターンがあることがわかります



[metricbeat01.png]



左ペインにある"Dashboard"をクリックします。@<br>{}
検索ウィンドウから"Metricbeat"を入力すると様々なDashboardがヒットします。



[metricbeat02.png]



今回は、"[Metricbeat System] Host Overview"というDashboardをクリックします。@<br>{}
CPUやメモリ、プロセスの状態をニアリアルタイムにモニタリングができていることがわかります。



[metricbeat03.png]



このようにサーバやコンテナなどにMetricbeatを導入することで一元的にモニタリングすることができます。@<br>{}
次が最後ですが、監査ログを簡単に取り込むための"Auditbeat"についてです。


== Auditbeat


サーバの監査としてauditdが出力する"audit.log"をモニタリングしている方は多くいるのではないでしょうか。@<br>{}
"audit.log"を保管するだけでなく、ニアリアルタイムにモニタリングするためにLogstashなどのツールを利用している方もいると思います。@<br>{}
ただ、これから"audit.log"をモニタリングしたいという人からしたらハードルが高く、モニタリングするまでに時間を要してしまいます。@<br>{}
そこで、Beatsには、Auditbeatというデータシッパーがあるので簡単に導入することができます。@<br>{}
ここまでFilbeatやMetricbeatを触ってきたらわかる通り、学習コストはほぼかからないでDashboardでモニタリングするところまでできてしまいます。  



それでは、ここからAuditbeatをインストールします。


//emlist[][bash]{
### Install Auditbeat
$ yum install auditbeat
//}


Auditbeatの設定ファイルは、以下を使用します。@<br>{}
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


データが取り込まれているかをKibanaを開いて確認します。@<br>{}
ブラウザを開いてKibanaへアクセスします。


//quote{
http://{Global_IP}:5601

//}


"Index Patterns"の画面を開くとFilebeatのインデックスパターンの他にAuditbeatのインデックスパターンがあることがわかります



[auditbeat01.png]



左ペインにある"Dashboard"をクリックします。@<br>{}
検索ウィンドウから"Auditbeat"を入力すると様々なDashboardがヒットします。



[auditbeat02.png]



"[Auditbeat File Integrity] Overview"や"[Auditbeat Auditd] Overview"からモニタリングが可能です。



[auditbeat03.png]



これまでBeatsを見てきていかがでしたか？@<br>{}
モニタリングしたいModuleを有効化するだけで簡単にモニタリングできる環境が手に入ります。@<br>{}
この他にもWidnowsを対象にしたものや、サービスの死活監視としてのBeatsなどがあります。@<br>{}
同じような学習コストで体験できるので、体験して頂ければと思います。  


= まとめ


いかがでしたか？@<br>{}
LogstashとBeatsの両方を体験することで、ログ収集時の選択肢が増えたのではないでしょうか。@<br>{}
また、LogstashとBeatsの違いがわからないという方々にお会いすることがあるので、少しでも違いを理解頂ければ幸いです。  



最後となりますが、ここまでお付き合い頂きありがとうございます。  



これからもみなさんがログと素敵な時間を過ごせることを願ってます。



@micci184


//footnote[1]["_grokparsefailure"は、grokフィルタでパースできない場合に発生します]

//footnote[2][AWSのSecurityGroup側で制限はかけているので、Kibana側では制限しないようにしています]
