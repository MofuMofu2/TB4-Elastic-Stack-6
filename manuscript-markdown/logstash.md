# Logstashを使ってみる

Logstashは、"Input""Filter""Output"の構成で記述します。  
Inputでログのソースを指定し、Filterで必要な要素に分解します。最後のOutputで出力先を指定します。  
今回は、AWS環境なので、Inputは、S3からログを取得して、OutputでElasticsearchにストアさせます。  

それでは、ここから実際にLogstashをインストールして、どのようなことを行うかを見ていきたいと思います。  

##  Logstashの実行環境を準備するよ

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

## Elasticsearchの設定ファイルの編集

Elasticsearchのファイル構成は以下です。

```bash
/etc/elasticsearch/
 ┣ elasticsearch.yml
 ┣ jvm.options
 ┗ log4j2.properties
```

elasticsearch.ymlでノード名や、クラスタ名の編集が可能です。  
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




