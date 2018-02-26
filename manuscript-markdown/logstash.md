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

ここからはElasticsearchの設定ファイルの編集します。

### Elasticsearchのディレクトリ構成について

Elasticsearchのディレクトリ構成は以下です。  
elasticsearch.ymlとjvm.optionsの設定を編集します。

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

### 


> Settings the heap size: https://www.elastic.co/guide/en/elasticsearch/reference/current/heap-size.html

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
| 2   | node.name                          | ノード名の指                                                           |
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

今回は、logstash.ymlの編集は行いません（後述で編集します）が、このファイルでどんなことができるかをサクッと書いておきます。  
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

公式に詳細が記載されているので参考にすると幸せになれるかもです。

> Settings File:
 https://www.elastic.co/guide/en/logstash/current/logstash-settings-file.html

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

コマンドラインで実行します。
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

```bash
$ vim /etc/logstash/alb.logs
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

追記した部分を以下で解説します。

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
       "message" => "https 2016-08-10T23:39:43.065466Z app/my-loadbalancer/50dc6c495c0c9188  192.168.131.39:2817 10.0.0.1:80 0.086 0.048 0.037 200 200 0 57 \"GET https://www.example.com:443/ HTTP/1.1\" \"curl/7.46.0\" ECDHE-RSA-AES128-GCM-SHA256 TLSv1.2  arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067 \"Root=1-58337281-1d84f3d73c47ec4e58577259\" www.example.com arn:aws:acm:us-east-2:123456789012:certificate/12345678-1234-1234-1234-123456789012",
      "@version" => "1",
          "host" => "ip-172-31-50-36"
}
```

標準入力で実行した時と同様に"message"に取り込んだログが出力されていることがわかります。
ただ、これでは構造化した形でElasticsearchにストアされないため、検索性が損なわれます。
その解決方法として"Filter"を利用します。

### Filterを使ってみる



先ほどまでは、"Input"と"Output"についての動きをみましたが、ここからは、"Filter"を使った動きをみたいと思います。






