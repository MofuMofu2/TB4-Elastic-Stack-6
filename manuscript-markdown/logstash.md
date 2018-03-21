# Logstashを使ってみる

## この章でやること

AWSでWebサイトを運営していて、ELBのログからどこの国からアクセスされているのかや、UserAgentを知りたいと行った時に、  
CloudWatchではモニタリングできないものがあります。  
でも大丈夫です！  
ELBはログを出力しているので、そのログをよしなに取り込んで、ビジュアライズすればいいだけなのです！  
ちなみに、ELBといっているけど、今回はALB（Application loadbalancer）を対象にするよ。

それでは、この章でやることです！

* ALB(AWSのアプリケーションロードバランサ)のログをLogstashでElasticsearchにストアできるになる
* 取り込んだログをKibanaでビジュアライズできる

## 実行環境を準備する

Logstashの使い方を知る前に、実行環境を整える必要があります。  
サーバは、AWSのEC2を利用しおり、OSは、AmazonLinuxで構築していきます。  
インスタンスタイプは、最低限必要なリソースを積んだものを選択しています。  
そのため、OS差異によって、発行するコマンドが変わってくるので、その辺は公式HPをみて頂ければと思います。

* Amazon Linux AMI 2017.09.1 (HVM), SSD Volume Type - ami-97785bed
* t2.medium(vCPU: 2,Mem: 4)

今回導入するミドルウェアのバージョンは以下です。

* Elasticsearch 6.2.2
* Logstash 6.2.2
* Kibana 6.2.2
* Metricbeat 6.2.2
* Auditbeat 6.2.2
* Packetbeat 6.2.2

[Download Elastic Stack](https://www.elastic.co/jp/products)

### こんな環境を構築するよ

[図を入れる]

### ALBのログを準備する

ALBのログを出力するには、ALB自体のロギングの設定を有効化する必要があります。  
有効化するとALBのログをS3のバケットにログを出力することができます。  

設定方法について説明はしないので、以下のAWSの公式HPを参考に実施して頂ければと思います。  

## ミドルウェアのインストールするよ

以下の流れでインストールしていきます。

1. Java 8インストール
2. Elasticsearchインストール
3. Logstashインストール
4. Kibanaインストール

### Java 8インストール

Elasticsearch、Logstashを実行するにあたって、Java 8が必要なため、インストールします。  
Javaの切り替えにalternativesコマンドを使用して変更します。

まずは、Javaがインストールされているかとバージョンを確認します。

```bash
$ java -version
xx
```

AmazonLinuxの場合は、Javaが最初からインストールされています。  
ただし、バージョンが、Java 7であることがわかります。  
そのため、Java 8をインストールしていきます。

```bash
### Install Java 8
$ sudo yum -y install java-1.8.0-openjdk-devel
```

Java 8のインストールが完了したので、再度バージョンを確認します。

```bash
$ java -version
xx
```

あれ？あれれ？  
バージョンが変わってないですね。。  

実は、Javaをインストールしただけでは切り替わらないため、alternativesコマンドを発行する必要があります。  
alternativesコマンドを発行すると対話形式でJavaの選択ができので、Java 8を選択します。  

```bash
$ sudo alternatives --config java

There are 2 programs which provide 'java'.

  Selection    Command
-----------------------------------------------
*+ 1           /usr/lib/jvm/jre-1.7.0-openjdk.x86_64/bin/java
   2           /usr/lib/jvm/jre-1.8.0-openjdk.x86_64/bin/java

Enter to keep the current selection[+], or type selection number: 2
```

それでは、再度バージョンを確認してみましょう。  
今度は、Java 8になっていることがわかりますね。

```bash
$ java -version
openjdk version "1.8.0_161"
OpenJDK Runtime Environment (build 1.8.0_161-b14)
OpenJDK 64-Bit Server VM (build 25.161-b14, mixed mode)
```

<!-- Mofu Javaの切り替えというの、もうすこし噛み砕いて説明した方がいいです。（一回java -versionを挟んでバージョン変わってないから変更する、など）ハマる人多い気がします。 -->

### Elasticsaerchインストール

ここからは、Elastic Stackのミドルウェアのインストールを実施していきます。  
ちなみに、公式HPをみるとわかりますが、英語ドキュメントです。  
やはり英語だと抵抗感を抱く人がいると思うので、できる限りわかりやすく日本語で書きます。  
正直、英語ぐらいわかるわー！って人は、飛ばしちゃってください。  

[Install Logstash](https://www.elastic.co/guide/en/logstash/current/installing-logstash.html)

てことで、Elasticsearchなどのパッケージをダウンロードするため、GPGキーをインポートします。

```bash
### Import GPG-Key
$ rpm --import https://artifacts.elastic.co/GPG-KEY-elasticsearch
```

キーの登録が完了したので、YUMリポジトリを追加します。  
"/etc/yum.repo/"配下に"elasticstack.repo"というファイルを作成します。  
公式では、logstash.repoとなっておりますが、今回はElasticsearchなどもインストールするため、Elastic Stackという名前にしました。  
要はファイル名は、任意で問題ないということです。

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

### Elasticsearchをインストール

Output先としてElasticsearchを利用するため、Elasticsearchをインストールします。

```bash
### Install Elasticsearch
$ sudo yum install elasticsearch
```

インストールが完了したので、バージョンを確認します。

```bash
$ /usr/share/elasticsearch/bin/elasticsearch --version
Version: 6.2.2, Build: 10b1edd/2018-02-16T19:01:30.685723Z, JVM: 1.8.0_161
```

Elasticsearchのサービス自動起動の設定をします。

```bash
### Auto start setting
$ sudo chkconfig --add elasticsearch
$ chkconfig --list | grep elasticsearch
elasticsearch  	0:off	1:off	2:on	3:on	4:on	5:on	6:off
```

<!-- Mofu Logstashの自動起動設定は入れないのでしょうか？ここは合わせた方がわかりやすいと思います。 -->

### Logstashをインストール

ログを取り込むのに必要なLogstashをインストールします。

```bash
### Install Logstash
$ sudo yum install logstash
```

インストールが完了したので、バージョンを確認します。

```bash
$ /usr/share/logstash/bin/logstash --version
logstash 6.2.2
```

今回は、ログのInput先がAWSのS3のため、プラグインをインストールする必要があります。  
それでは、"S3 Input Plugin"をインストールします。

[S3 Input Plugin](https://www.elastic.co/guide/en/logstash/current/plugins-inputs-s3.html)

```bash
### Install S3 Input Plugin
$ /usr/share/logstash/bin/logstash-plugin install logstash-input-s3
Validating logstash-input-s3
Installing logstash-input-s3
Installation successful
```

LogstashもElasticsearchと同様にサービス自動起動の設定をします。  

```bash
hogehoge
```

### Kibanaをインストール

ビジュアライズするためにKibanaをインストールします。

```bash
### Install Kibana
$ yum install kibana
```

Kibanaも自動起動設定をします。

```bash
### Auto start setting
$ sudo chkconfig --add kibana
$ chkconfig --list | grep kibana
kibana  	0:off	1:off	2:on	3:on	4:on	5:on	6:off
```

これで全てインストールが完了しました。  

## ミドルウェアの設定

ここから必要な設定を実施していきます。  

以下の流れでミドルウェアの設定をしていきます。

1. Elastcisearchの設定
2. 

### Elasticsearchの設定について

設定変更する前に、Elasticsaerchの設定ファイルが構成されているディレクトリを見ていきたいと思います。  

```bash
### Elasticsearch directory structure
/etc/elasticsearch/
 ┣ elasticsearch.yml
 ┣ jvm.options
 ┗ log4j2.properties
```

"/etc/elasticserch/"配下に3つのファイルが配置されてます。  
Elasticsearchを構成する際にjvm.optionsとelasticsearch.ymlを主に設定します。  
log4j2.propertiesは、ログの出力形式など変更が必要な際に設定してください。

今回、設定変更するのは、jvm.optionsとelasticserch.ymlです。  
この二つの設定ファイルの変更と設定についての考慮点などを記載したいと思います。

#### jvm.optionsという設定ファイルについて

Elasticsaerchのヒープサイズを変更したい！ってなった時は、jvm.optionsで設定変更ができます。   
例えば、ヒープサイズの最大と最小を設定する場合は、"Xms(minimum heap size)"と"Xmx(maximum heap size) "を変更します。  
じゃあ、いくつに設定すればいいの？と思う方もいるかと思いますが、要件によって変わってくる項目です。  
しかし、Elasticsaerch社の公式HPで掲載されているので、その内容を以下に記載するので参考に設定して頂ければ良いかと思います。  

* 最小ヒープサイズ(Xms)と最大ヒープサイズ(Xmx)の値を等しくする
* ヒープサイズを上げすぎるとGCの休止をまねく可能性がある
* Xmxは、物理メモリの50%を割り当てて、ファイルシステム側にも十分に残すようにする
* 割り当てるメモリは、32GB以下にする

今回のサーバは、メモリを4GB搭載しているので2GBをわりあてるかたちでいいかと思います。  
以下のように設定します。

```bash
### Heap size change
$ vim /etc/elasticsearch/jvm.options
-Xms2g
-Xmx2g
```

[Settings the heap size:](https://www.elastic.co/guide/en/elasticsearch/reference/current/heap-size.html)

#### elasticserch.ymlという設定ファイルについて

Elasticsaerchでクラスタ構成をする場合などに設定するファイルです。  
今回は、クラスタ構成はしないので、以下のアクセス元制限の設定のみを行います。  

実際に変更します。  
この設定は、どこからでもアクセス可能とするため、"network.host"のみを編集します。
"0.0.0.0"と設定することで、どこからでもアクセス可能となります。

```bash
### Settings to enable access from anywhere
$ network.host: 0.0.0.0
```

以下にelasticsearch.ymlの設定項目について表に記載しています。  
ご興味がある方は、参考にして頂ければと思います。

| No. | Item                               | Content                                                                |
|:----|:-----------------------------------|:-----------------------------------------------------------------------|
| 1   | cluster.name: my-application       | クラスタ名の指定                                                       |
| 2   | node.name                          | ノード名の指定                                                         |
| 3   | network.host                       | アクセス元のネットワークアドレスを指定することで制限をかけることが可能 |
| 4   | discovery.zen.ping.unicast.hosts   | クラスタを組む際にノードを指定                                         |
| 5   | discovery.zen.minimum_master_nodes | 必要最低限のノード数を指定                                             |


#### Elasticsaerchサービス起動

Elasticsearchを起動し、動作確認をします。

```bash
### Service activation
$ service elasticsearch start
Starting elasticsearch:                                    [  OK  ]
```

動作確認としてELasticsearchに対して、curlします。  
Elasticsearchは、ローカル環境に構築しているので、"localhost"にcurlします。  
デフォルトのポートは、"9200"のため、ポート指定します。

```bash
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

Elasticsaerchからレスポンスが返ってきましたね。  
これでElasticsearchの設定完了です。

## Logstashの設定について

Elasticsearchの時と同様にLogstashもディレクトリ構成をみていきたいと思います。  

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

ここから個々のファイルについてと説明と設定を行なっていきます。  
細かい設定などがありますが、ちょっと頑張ってもらえればと思います。

### logstash.ymlの編集

今回は、logstash.ymlの編集は行いません。  
なので、飛ばしても大丈夫ですし、ご興味のある方は、読み進めてもらえればと思います。

logstash.ymlでは、パイプラインのバッチサイズやディレイ設定が可能です。  
要は、Logstashの動作についてのハンドリングを施すことが可能な設定ファイルです。  
ymlファイルのため、以下のように階層やフラットな構成で記載することが可能です。

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

そもそも、パイプラインってなんぞ？って人がいると思うので説明しますね。  
そもそもですが、Logstashは、"Input"、"Filter"、"Output"の3つで構成されています。  
どんな役割かを以下に記載します。

* Input: データソースを指定し、アクセスし、ログを取得します
* Filter: 取得したログを構造化するため、Grokでキーバリューに分割したり、地理情報などを付与したり様々なフィルタを施します
* Output: データの取り込み先を指定します（今回はElasticsearchを指定しています）

Logstashに構成されている"Input"、"Filter"、"Output"の一連をパイプラインと言っております。  
また、この定義するためのファイルが、パイプラインファイルです。

### Logstashを動かす

実際にLogstashを動かすためにパイプラインファイルを設定して、動かしていきたいと思います。  
Logstashの起動方法は、"コマンド起動"と"サービス起動"の二つの方法があります。  
最終的には、"サービス起動"で動かしますが、初めは慣れるためにも"コマンド起動"で行なっていきます。

#### Logstashはじめの一歩

早速ですが、パイプラインファイルを作成します。  
このパイプラインは、単純に標準入力から標準出力するものです。  
そのため、"Input"と"Output"のみの構成としています。

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

"test.conf"ができましたね。  
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

気づいた方もいるかと思いますが、パイプラインファイルに"Filter"を記載していません。  
"Filter"を記載せず、"Input"と"Output"のみで構成することが可能なのです。  
ただし、"Filter"がないため、入力データが何も加工されず、出力されます。

#### ALBのログをLogstashで取り込む

ここからは実際のログを利用してパイプラインを扱っていきたいと思います。  
対象のログとして、AWSのALBログを利用します。

ALBのログは、AWS公式ページに記載されているサンプルログを利用します。

[Access Logs for Your Application Load Balancer:](https://docs.aws.amazon.com/elasticloadbalancing/latest/application/load-balancer-access-logs.html)

以下がサンプルログです。

```
https 2016-08-10T23:39:43.065466Z app/my-loadbalancer/50dc6c495c0c9188 
192.168.131.39:2817 10.0.0.1:80 0.086 0.048 0.037 200 200 0 57 
"GET https://www.example.com:443/ HTTP/1.1" "curl/7.46.0" ECDHE-RSA-AES128-GCM-SHA256 TLSv1.2 
arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067
"Root=1-58337281-1d84f3d73c47ec4e58577259" www.example.com arn:aws:acm:us-east-2:123456789012:certificate/12345678-1234-1234-1234-123456789012
```

このサンプルログを"/etc/logstash/"配下に配置します。  
ファイル名は任意でいいのですが、今回は、"alb.log"にします。

```bash
$ vim /etc/logstash/alb.log
https 2016-08-10T23:39:43.065466Z app/my-loadbalancer/50dc6c495c0c9188  192.168.131.39:2817 10.0.0.1:80 0.086 0.048 0.037 200 200 0 57 "GET https://www.example.com:443/ HTTP/1.1" "curl/7.46.0" ECDHE-RSA-AES128-GCM-SHA256 TLSv1.2  arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067 "Root=1-58337281-1d84f3d73c47ec4e58577259" www.example.com arn:aws:acm:us-east-2:123456789012:certificate/12345678-1234-1234-1234-123456789012
```

ログファイルの準備が整ったので、パイプラインファイルを新しく作成します。  
先程作成したtest.confは、"Input"を標準入力としてましたが、ファイルを取り込むので"File input plugin"を使用します。  
"File input plugin"は、標準でインストールされているので、インストールは不要です。

それでは、"alb.conf"という設定ファイルを作成します。

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

追記した部分をについて表で説明します。

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
ただ、これでは構造化した形でElasticsearchにストアされないため、検索性が損なわれます。（"message"というキーに全てのログの全てのデータが入ってしまっているので、意味をなしていないということです）  
そこで、解決方法として"Filter"を利用します。

### LogstashのFilterを使ってみる

"Filter"では、取得したログを正規表現でパースするGrokフィルタや、地理情報を得るためのGeoIPフィルタを施すことができます。  
今回のALBもGrokフィルタなどを使うことで構造化することが可能です。  

とはいえ、どのように構造化すればいいのかということもあるので、まずはALBのログフォーマットを把握する必要があります。  
以下にALBのログフォーマットを記載します。

```bash
type timestamp elb client:port target:port request_processing_time target_processing_time response_processing_time elb_status_code target_status_code received_bytes sent_bytes "request" "user_agent" ssl_cipher ssl_protocol target_group_arn trace_id domain_name chosen_cert_arn
```

各フィールドを表にまとめると以下になります。  
このようにログを取り込む前にログフォーマットを確認し、フィールド名を定義します。  
また、"Type"で各フィールドの型を定義します。  

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

定義したフィールド単位で分割されるようにGrokフィルタを利用し、分割します。  
ちなみにですが、Grokフィルタは、様々なログに合わせて正規表現でkey-value形式に加工することが可能です。  
Grokフィルタするためのパターンファイルを作成します。

が、その前にパターンファイルを格納するディレクトリを作成します。  
パターンファイルを作成せずにパイプラインファイルのFilter内にGrokフィルタを記載することも可能ですが、可読性や管理がしやすくするためパターンファイルを外出ししています。

```bash
### Create directory
$ mkdir /etc/logstash/patterns
$ ll | grep patterns
drwxr-xr-x 2 root root 4096 xxx xx xx:xx patterns
```

patternsディレクトリが作成できたので、配下にALBのパターンファイルを作成します。  
中身については、闇深いのでここでは説明しません。。無邪気に貼っつけてください。  
また、Typeは、インデックステンプレートで作成するのが一般的かと思いますが、今回は、パターンファイルの中で指定します（いろんなやり方があるんだよという意味で）

あ、後述で"grok-filter"の説明でもありますが、このパターンファイルを呼び出す時は、ファイル名の指定だけでなく、Grok-Patternsの指定も必要になります。  
ここでいう"Grok-Patterns"は、"ALB_ACCESS_LOG"に当ります。  
この"ALB_ACCESS_LOG"は、任意の名前を指定できます。

```bash
$ sudo vim /etc/logstash/patterns/alb_patterns
# Application Load Balancing
ALB_ACCESS_LOG %{NOTSPACE:class} %{TIMESTAMP_ISO8601:date} %{NOTSPACE:elb}  (?:%{IP:client_ip}:%{INT:client_port:int}) (?:%{IP:backend_ip}:%{INT:backend_port:int}|-) (:?%{NUMBER:request_processing_time:float}|-1) (?:%{NUMBER:target_processing_time:float}|-1) (?:%{NUMBER:response_processing_time:float}|-1) (?:%{INT:elb_status_code}|-) (?:%{INT:target_status_code:int}|-) %{INT:received_bytes:int} %{INT:sent_bytes:int} \"%{ELB_REQUEST_LINE}\" \"(?:%{DATA:user_agent}|-)\" (?:%{NOTSPACE:ssl_cipher}|-) (?:%{NOTSPACE:ssl_protocol}|-)  %{NOTSPACE:target_group_arn} \"%{NOTSPACE:trace_id}\"
```

パターンファイルが準備できましたので、パイプライファイルの"alb.conf"に"Filter"を追加します。

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
設定内容については、後述で説明しますので、無邪気に実行してみてください。  
先ほど実行した時と違って、いい感じにkey-valueのかたちになっていることがわかります。

```bash
$ /usr/share/logstash/bin/logstash -f /etc/logstash/conf.d/alb.conf
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
今回使用しているフィルタは、以下の3つです。

1. grok-filter
2. date-filter
3. geoip-filter

1. grok-filter

"grok-filter"についてですが、先ほども説明した通り正規表現でパースする際に使用します。  
"patterns_dir"で外だししているパターンファイルを呼び出すことができます。  
また、"match"で"message"に取り込まれている値を対象にGrok-Patterns(ここでいうALB_ACCESS_LOG)を適用しています。

```bash
### grok-filter
  grok {
    patterns_dir => ["/etc/logstash/patterns/alb_patterns"]
    match => { "message" => "%{ALB_ACCESS_LOG}" }
  }
```

2. date-filter

"date-filter"で"実際のログが出力された時間を"@timestamp"に置き換えています。  
置き換えないとLogstashが取り込んだ時刻が"@timestamp"に記録されてしまうからです。そのため、@timestamp"を"grok-filter"で抽出した"date"で置き換えます。
また、タイムゾーンを日本にしたいため、"Asia/Tokyo"を指定しています。

```bash
  date {
    match => [ "date", "ISO8601" ]
    timezone => "Asia/Tokyo"
    target => "@timestamp"
  }
```

3. geoip-filter

"geoip-filter"を使用することでIPアドレスから地理情報を取得することが可能です。  
例えば、どこかのグローバルIPアドレスからWhoisでどこの国からのアクセスかな？って調べる時があると思います。  
その動作を一つひとつのログに対してやっていたら死んでしまいます。。なので、"geoip-filter"を使用すれば、自動で地理情報を付与してくれるのです。  
ちなみにですが、地理情報は、Logstashが内部で保持しているデータベースを照合して地理情報を付与してくれています[^1]

[^1]: 地理情報の精度を上げたい場合は、有料版のデータをインポートする必要があります

"geoip-filter"を適用するフィールドを指定します。  
今回は、クライアントのIPアドレを元にどこからアクセスされているかを知りたいため、フィールド名の"client_ip"を指定します。  
設定方法は、以下です。

```bash
  geoip {
    source => "client_ip"
  }
```

他にも"Filter"でやれることはたくさんあります。  
"mutate-filter"を使用すれば、不要なフィールドの削除を行ったりもできます。  
例えば、messageの値は、全てkey-valueで分割されてストアされています。そのため、無駄なリソースを使いたくない場合は、削除といったことも可能です。  
個人的には、ストアされたデータで"_grokparsefailure"が発生した時の場合も踏まえると、残した方がいいと思ってます。[^2]

[^2]: "_grokparsefailure"は、grokフィルタでパースできない場合に発生します

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


また、パイプラインを実行するWoker数を変更することも可能です。
変更する際は、"pipeline.workers"の数を変更します。
Woker数の目安は、割り当てたいCPUコア数とイコールにするのが良いです。

```bash
### Change Woker
pipeline.workers: 2
```

