= Logstashを使ってみる


AWSを利用してWebサイトを運営しているとき、ELBのアクセスログを用いてアクセス元の国やUserAgentを知りたくなることが
あるかもしれません。しかし、これらの情報の中にはCloudWatchではモニタリングできないものがあります。

でも大丈夫です！
ELBはログを出力しているので、そのログを何らかの形で取得し可視化すれば良いのです！
ちなみに、今回はALB（Application loadbalancer）からデータを取得します。

この章で目指すことは次の2点です。

 * ALB（AWSのアプリケーションロードバランサ）のログをLogstashからElasticsearchに保存する
 * Elasticsearchに保存したログをKibanaでビジュアライズできるようになる


== 実行環境を準備する

Logstashの使い方を知る前に、実行環境を整える必要があります。
サーバはAWSのEC2を利用し、OSはAmazonLinuxで構築していきます。
インスタンスタイプは、稼働に最低限必要なリソースのものを選択しています。
OSによって発行するコマンドが変わってくるので、詳しくは公式HPを確認してください。

 * Amazon Linux AMI 2017.09.1 (HVM), SSD Volume Type - ami-97785bed
 * t2.medium(vCPU: 2,Mem: 4)

今回導入するミドルウェアのバージョンは次の通りです。

 * Elasticsearch 6.2.2
 * Logstash 6.2.2
 * Kibana 6.2.2
 * Metricbeat 6.2.2
 * Auditbeat 6.2.2
 * Packetbeat 6.2.2

各プロダクトはこちらのリンク（@<href>{https://www.elastic.co/jp/products}）から
ダウンロードすることが可能です。

=== 想定される環境


ユーザがWebサイトにアクセスした際に、ALBで出力したアクセスログをS3に保存します。
S3に保存されたアクセスログを、Logstashが定期的に取得する構成です。

//image[logstash01][本章で想定している環境構成]{
//}

=== ALBのログを準備する

ALBのログを出力するために、ALB自体のロギングの設定を有効化する必要があります。
これにより、ALBのログをS3のバケットに出力できます。

ALB自体のロギング設定・S3バケットの設定方法については本文では解説しません。
AWSの公式ドキュメントなどを参考に設定してください。


== ミドルウェアのインストール

次の順番でインストールします。

 1. Java（バージョン8）
 2. Elasticsearch
 3. Logstash
 4. Kibana

=== Java 8のインストール

Elasticsearch、Logstashの動作にはJava（バージョン8）が必要です。
まずは、Javaがインストールされているかを確認します。またインストールされている場合は、Javaのバージョンを確認します。

//list[logstash-01][Javaのバージョンを確認する]{
java -version
//}

AmazonLinuxの場合、Javaが最初からインストールされています。
ただしバージョンが7のため、Java 8を新しくインストールする必要があります。

//list[logstash-02][Java 8のインストール]{
sudo yum -y install java-1.8.0-openjdk-devel
//}

=== Javaのバージョンを変更

Javaをインストールしただけでは、OSが利用するJavaのバージョンが切り替わりません。
@<code>{alternatives}コマンドを利用して利用するJavaのバージョンを切り替えましょう。


//list[logstash-03][Javaのバージョンを変更]{
sudo alternatives --config java
//}

//cmd{
$ sudo alternatives --config java

There are 2 programs which provide 'java'.

  Selection    Command
-----------------------------------------------
*+ 1           /usr/lib/jvm/jre-1.7.0-openjdk.x86_64/bin/java
   2           /usr/lib/jvm/jre-1.8.0-openjdk.x86_64/bin/java

Enter to keep the current selection[+], or type selection number: 2
//}

Javaのバージョンを変更した後は、もう一度@<code>{java -version}でバージョンが8になっているか確認しましょう。

//cmd{
$ java -version
openjdk version "1.8.0_161"
OpenJDK Runtime Environment (build 1.8.0_161-b14)
OpenJDK 64-Bit Server VM (build 25.161-b14, mixed mode)
//}

=== Elasticsearchのインストール


ここからは、Elastic Stackのインストールを行います。
ちなみに、公式ドキュメントは英語（@<href>{https://www.elastic.co/guide/en/logstash/current/installing-logstash.html,Install Logstash}
）です。
やはり英語だと抵抗感を抱く人がいると思うので、本章ではできる限りわかりやすく日本語で説明します。
英語のドキュメントで問題ない方はこの項は読み飛ばしていただいても問題ありません。


始めに、Elasticsearchなどのパッケージをダウンロードするため、GPGキーをインポートします。

//list[logstash-GPG][GPGキーのインポート]{
rpm --import https://artifacts.elastic.co/GPG-KEY-elasticsearch
//}


キーの登録が完了したので、YUMリポジトリを追加します。
@<code>{/etc/yum.repo/}配下に@<code>{elasticstack.repo}というファイルを作成します。
公式ドキュメントではlogstash.repoとなっていますが、今回はElasticsearchなども一緒にインストールするため、elasticstack.repoという名前にしました。
ファイル名は自由につけてよい、ということです。


//list[logstash-04][elasticstack.repoの追加]{
sudo vim /etc/yum.repos.d/elasticstack.repo
//}

//cmd{
[elasticstack-6.x]
name=Elastic repository for 6.x packages
baseurl=https://artifacts.elastic.co/packages/6.x/yum
gpgcheck=1
gpgkey=https://artifacts.elastic.co/GPG-KEY-elasticsearch
enabled=1
autorefresh=1
type=rpm-md
//}

Output先としてElasticsearchを利用するため、Elasticsearchをインストールします。


//list[logstash-05][Elasticsearchのインストール]{
sudo yum install elasticsearch
//}


Elasticsearchのバージョンを念のため確認します。


//list[logstash-06][Elasticsearhのバージョン確認]{
/usr/share/elasticsearch/bin/elasticsearch --version
//}

//cmd{
$ /usr/share/elasticsearch/bin/elasticsearch --version
Version: 6.2.2, Build: 10b1edd/2018-02-16T19:01:30.685723Z, JVM: 1.8.0_161
//}


Elasticsearchのサービス自動起動の設定をします。


//list[logstash-07][サービス自動起動の設定]{
sudo chkconfig --add elasticsearch
//}

//cmd{
$ sudo chkconfig --add elasticsearch
$ chkconfig --list | grep elasticsearch
elasticsearch   0:off   1:off   2:on    3:on    4:on    5:on    6:off
//}


=== Logstashのインストール

ALBのログを取得するため、Logstashをインストールします。


//list[logstash-08][Logstashのインストール]{
sudo yum install logstash
//}


インストールが完了したので、Logstashのバージョンを確認します。


//list[logstash-09][Logstashのバージョン確認]{
/usr/share/logstash/bin/logstash --version
//}


今回はログの取得元をAWSのs3としています。s3からログを取得するには追加でプラグインをインストールする必要があります。
それでは、@<code>{S3 Input Plugin}（@<href>{https://www.elastic.co/guide/en/logstash/current/plugins-inputs-s3.html}
）をインストールします。

//list[logstash-10][S3 Input Pluginのインストール]{
/usr/share/logstash/bin/logstash-plugin install logstash-input-s3
//}

//cmd{
$ /usr/share/logstash/bin/logstash-plugin install logstash-input-s3
Validating logstash-input-s3
Installing logstash-input-s3
Installation successful
//}


LogstashもElasticsearchと同様、サービス自動起動の設定をしておくと良いでしょう。

//list[logstash-13][Logstashの自動起動設定]{
sudo chkconfig --add logstash
//}

//cmd{
$ sudo chkconfig --add logstash
$ chkconfig --list | grep logstash
logstash      0:off   1:off   2:on    3:on    4:on    5:on    6:off

//}


=== Kibanaのインストール


取得したデータを可視化するため、Kibanaをインストールします。

//list[logstash-11][Kibanaのインストール]{
yum install kibana
//}

Kibanaも他のミドルウェアと同様に、サービス自動起動の設定を行います。

//list[logstash-12][Kibanaの自動起動設定]{
sudo chkconfig --add kibana
//}

//cmd{
$ sudo chkconfig --add kibana
$ chkconfig --list | grep kibana
kibana      0:off   1:off   2:on    3:on    4:on    5:on    6:off
//}

これで全てのインストールが完了しました。


== ミドルウェアの設定


ここからミドルウェアに対して、必要な設定を行います。
以下の流れでミドルウェアの設定をします。

 1. Elastcisearchの設定
 2. Logstshの設定
 3. Kibanaの設定



=== Elasticsearchの環境準備


設定を変更する前に、Elasticsearchの設定ファイルが構成されているディレクトリの内容を確認しましょう。

//cmd{
/etc/elasticsearch/
 ┣ elasticsearch.yml
 ┣ jvm.options
 ┗ log4j2.properties
//}

@<code>{/etc/elasticsearch}配下に3つのファイルが配置されてます。
Elasticsearchを構成する際には@<code>{jvm.options}と@<code>{elasticsearch.yml}を主に設定します。
@<code>{log4j2.properties}は、ログの出力形式など変更が必要な際に設定してください。


今回は@<code>{jvm.options}と@<code>{elasticsearch.yml}を編集します。
この二つの設定ファイルの変更と設定について考慮が必要な点などを記載したいと思います。


==== jvm.optionsについて

Elasticsearchのヒープサイズを変更したい場合、jvm.optionsを編集します。
例えば、ヒープサイズの最大と最小を設定する場合は、@<code>{Xms(minimum heap size)}と@<code>{Xmx(maximum heap size)}を変更します。
いくつに設定すればいいの？と思う方もいるかと思いますが、これは要件によって変わってくる項目です。
公式ドキュメント（@<href>{https://www.elastic.co/guide/en/elasticsearch/reference/current/heap-size.html,Settings the heap size:}）にも考慮点が記載されているので、そちらも参考に値を決めてください。

 * 最小ヒープサイズ(Xms)と最大ヒープサイズ(Xmx)の値を等しくする
 * ヒープサイズを上げすぎるとGCの休止を招く可能性がある
 * Xmxは、物理メモリの50%を割り当てて、ファイルシステム側にも十分に残すようにする
 * 割り当てるメモリは、32GB以下にする

今回のサーバは、メモリを4GB搭載しているので2GBをElasticsearchに割り当てます。


//list[logstash-14][Elasticsearhのヒープサイズを設定]{
vim /etc/elasticsearch/jvm.options
# 下記設定に変更
-Xms2g
-Xmx2g
//}

==== Elasticsearch.ymlについて

Elasticsearchでクラスタ構成をする場合などに設定するファイルです。
今回クラスタ構成はしないので、アクセス元制限の設定のみを行います。

@<code>{network.host}を@<code>{0.0.0.0}に編集します。
これで、どこからでもElasticsearchにアクセスできるようになります。

//list[logstash-15][アクセス元IPの設定]{
network.host: 0.0.0.0
//}

主な設定項目を@<table>{logstash-16}にまとめていますので、必要に応じて設定を変更してください。

//table[logstash-16][elasticsearch.ymlの設定項目]{
No.	Item	Content
-----------------
1	cluster.name: my-application	クラスタ名の指定
2	node.name	ノード名の指定
3	network.host	アクセス元のネットワークアドレスを指定することで制限をかける
4	discovery.zen.ping.unicast.hosts	クラスタを組む際にノードを指定
5	discovery.zen.minimum.master.nodes	必要最低限のノード数を指定
//}

==== Elasticsearchサービス起動

Elasticsearchを起動し、動作確認をします。


//list[logstash-17][Elasticsearhの起動]{
service elasticsearch start
//}


動作確認としてElasticsearchに対して、@<code>{curl}コマンドを発行します。
Elasticsearchは、ローカル環境に構築しているので@<code>{localhost}を接続先とします。
ポートは設定を変更していない限り@<code>{9200}です。今回はデフォルト設定のままです。

//list[logstash-18][Elasticsearhへ接続]{
curl localhost:9200
//}


Elasticsearchからレスポンスが返ってきましたね。
これでElasticsearchの設定は完了です。


=== Logstashの環境準備


Elasticsearchの時と同様に、Logstashもディレクトリ構成を確認します。


//cmd{
/etc/logstash/
 ┣ conf.d
 ┣ jvm.options
 ┣ log4j2.properties
 ┣ logstash.yml
 ┣ pipelines.yml
 ┗ startup.options
//}


各ファイルやディレクトリについて説明します。

//table[logstash-19][]{
No.	Item	Content
-----------------
1	conf.d	Input/Filter/Outputのパイプラインを記載したファイルの格納場所
2	jvm.options	jvmオプションの管理ファイル
3	log4j2.properties	log4jのプロパティ管理ファイル
4	logstash.yml	Logstashの設定ファイル
5	pipelines.yml	パイプラインを複数実行する際に定義するファイル
6	startup.options	Logstash起動時に利用されるファイル
//}


==== logstash.ymlの編集

今回はlogstash.ymlの編集を行いませんが、@<code>{logstash.yml}の役割について解説します。

このファイルでは、パイプラインのバッチサイズやディレイ設定を行います。
Logstashの動作についてのハンドリングをすることが可能です。
ymlファイルのため、階層がフラットな構成で記述できます。

//list[logstash-20][logstash.yml]{
# hierarchical form
pipeline:
  batch:
    size: 125
    delay: 50
# flat key
pipeline.batch.size: 125
pipeline.batch.delay: 50
//}


Logstashは@<code>{Input}、@<code>{Filter}、@<code>{Output}の3つで構成されています。

 * Input: データの取得元を設定します。
 * Filter: データを構造化するための処理内容を設定します。データの追加・削除も可能です。
 * Output: データの送信先を指定します。（今回はElasticsearchを指定しています）

この一連の流れのことを@<b>{パイプライン（pipeline）}といいます。

==== Logstashのパイプラインを実行する


Logstashの起動方法は、コマンド起動とサービス起動の2種類が存在します。
最終的にはサービス起動を利用したほうが利便性も高いのですが、最初はコマンド起動を利用してLogstashの操作に慣れると良いでしょう。

Logstashを起動するため、パイプラインファイルを作成します。
このパイプラインは、単純に標準入力からLogstashを通して標準出力を行うものです。
そのため、InputとOutputのみの構成としています。

//list[logstash-21][test.confの作成]{
input {
  stdin { }
}
output {
  stdout { codec => rubydebug }
}
//}

パイプラインファイルは@<code>{test.conf}として保存し、@<code>{/etc/logstash/conf.d/}に配置します。

//list[logstash-22][Logstashの起動]{
/usr/share/logstash/bin/logstash -f /etc/logstash/conf.d/test.conf
//}

Logstashを起動後、任意の文字を標準入力します。
入力した文字（ここではtest）がmessageに表示されればLogstashは起動しています。

パイプラインファイルにFilterの記載は必須ではありません。InputとOutputのみで構成することが可能なのです。
ただしこの場合、入力データの加工はできません。


==== ALBのログをLogstashで取り込む


ここからはALBのログを利用してパイプラインを扱っていきたいと思います。
ALBのログは、AWS公式ページ（@<href>{https://docs.aws.amazon.com/elasticloadbalancing/latest/application/load-balancer-access-logs.html,Access Logs for Your Application Load Balancer:}
）に記載されているサンプルログを利用します。

//list[logstash-23][ALBのサンプルログ]{
https 2016-08-10T23:39:43.065466Z app/my-loadbalancer/50dc6c495c0c9188
192.168.131.39:2817 10.0.0.1:80 0.086 0.048 0.037 200 200 0 57
"GET https://www.example.com:443/ HTTP/1.1" "curl/7.46.0" ECDHE-RSA-AES128-GCM-SHA256 TLSv1.2
arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067
"Root=1-58337281-1d84f3d73c47ec4e58577259" www.example.com
arn:aws:acm:us-east-2:123456789012:certificate/12345678-1234-1234-1234-123456789012
//}


このサンプルログを@<code>{/etc/logstash/}配下に@<code>{alb.log}として保存します。ファイル名は任意です。

ログファイルの準備が整ったので、パイプラインファイルを新しく作成します。
先程作成したtest.confは、Inputを標準入力としていました。

今回はファイルを取り込むので@<code>{File input plugin}を使用します。
このプラグインは標準でインストールされているので、インストールは不要です。

新しく@<code>{alb.conf}という名前でパイプラインファイルを作成します。


//list[logstash-24][alb.conf]{
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


追記した部分について表で説明します。

//table[logstash-25][編集部分]{
No.	Item	Content
-----------------
1	path	取り込むファイルを指定します(ディレクトリ指定の"*"指定も可能)
2	start_position	Logstashを起動した時にどこから読み込むかの指定(デフォルトはend)
3	sincedb_path	ログファイルを前回どこまで取り込んだかを記載するファイル
//}

@<code>{alb.conf}を引数にLogstashを起動します。

//list[logstash-26][Logstashの起動]{
/usr/share/logstash/bin/logstash -f /etc/logstash/conf.d/alb.conf
//}

//cmd{
$ /usr/share/logstash/bin/logstash -f /etc/logstash/conf.d/alb.conf
{
    "@timestamp" => 2018-02-26T08:15:31.322Z,
          "path" => "/etc/logstash/alb.logs",
       "message" => "https 2016-08-10T23:39:43.065466Z app/my-loadbalancer/50dc6c495c0c9188  192.168.131.39:2817 10.0.0.1:80 0.086 0.048 0.037 200 200 0 57 "GET https://www.example.com:443/ HTTP/1.1" "curl/7.46.0" ECDHE-RSA-AES128-GCM-SHA256 TLSv1.2  arn:aws:elasticloadbalancing:us-east-2:123456789012:targetgroup/my-targets/73e2d6bc24d8a067 "Root=1-58337281-1d84f3d73c47ec4e58577259" www.example.com arn:aws:acm:us-east-2:123456789012:certificate/12345678-1234-1234-1234-123456789012,
      "@version" => "1",
          "host" => "ip-xxx-xx-Xx-xx"
}
//}


標準入力で実行した時と同様にmessageに取り込んだログが出力されていることがわかります。
ただ、これでは構造化した形でElasticsearchにデータ転送できないので、検索性が損なわれます。
messageというキーに全てのログの全てのデータが入ってしまうと、Kibanaで検索する際に不都合が発生するのです。
そこで、Filterを利用してmessageからデータを分割していきます。


==== LogstashのFilterを使ってみる


取得したログを正規表現でパースするためのGrokフィルタや、地理情報を得るためのGeoIPフィルタなど、
Filterにはログの種別に合わせた処理をするためのプラグインが存在します。
今回のALBもGrokフィルタなどを使うことで構造化したほうが良いでしょう。

とはいえ、どのように構造化すればいいのか迷ってしまいます。まずはALBのログフォーマットを把握し、作戦を立てます。

各フィールドを@<table>{table01}にまとめました。
このようにログを取り込む前にログフォーマットを確認し、フィールド名を定義します。
また、@<code>{Type}で各フィールドの型を定義しています。

//table[table01][ALBのログフォーマットとデータ型]{
Log	Field	Type
-----------------
type	class	string
timestamp	date	date
elb	elb	string
client_ip	client_ip	int
client_port	target_port	int
target_ip	target_ip	int
target_port	target_port	int
request	processing time request processing time	float
target processing time	target processing time	float
response processing time	response processing time	float
elb status code	elb status code	string
target status code	target status code	string
received_bytes	received_bytes	int
sent_bytes	sent_bytes	int
request	ELB REQUEST LINE	string
user_agent	user_agent	string
ssl_cipher	ssl_cipher	string
ssl_protocol	ssl_protocol	string
target group arn	target group arn	string
trace_id	trace_id	string
//}


定義したフィールド単位で分割したいので @<code>{Grok}フィルタを利用します。
Grokフィルタは正規表現でデータやログをkey-value形式に加工することが可能です。

パターンファイルを格納するディレクトリを作成します。
パターンファイルを作成せずにパイプラインファイルのFilter内にGrokフィルタを記載することも可能ですが、可読性や管理を楽にするためパターンファイルを外出ししています。

//cmd{
$ mkdir /etc/logstash/patterns
$ ll | grep patterns
drwxr-xr-x 2 root root 4096 xxx xx xx:xx patterns
//}

ディレクトリが作成できたので、ALBのパターンファイルを作成します。
中身についての詳細？？？？章で解説しています。
また、Typeは、インデックステンプレートで作成するのが一般的かと思いますが、今回は、パターンファイルの中で指定します（いろんなやり方があるんだよという意味で）

#@# ここ、？？？章、とした部分、追加で加えた部分に入っていますか？であればそこを参照したほうがいいですね

このパターンファイルを呼び出す時は、ファイル名の指定だけでなく@<code>{Grok-Patterns}の指定も必要です。
ここでいう@<code>{Grok-Patterns}は、@<code>{ALB ACCESS LOG}に該当します。
この@<code>{ALB ACCESS LOG}は、任意の名前を指定することができます。


//list[logstash-29][/etc/logstash/patterns/alb_patternsを次の通り編集]{
ALB_ACCESS_LOG %{NOTSPACE:class} %{TIMESTAMP_ISO8601:date} %{NOTSPACE:elb}
(?:%{IP:client_ip}:%{INT:client_port:int}) (?:%{IP:backend_ip}:%{INT:backend_port:int}|-)
(:?%{NUMBER:request_processing_time:float}|-1) (?:%{NUMBER:target_processing_time:float}|-1)
(?:%{NUMBER:response_processing_time:float}|-1) （紙面の都合により改行）
(?:%{INT:elb_status_code}|-) (?:%{INT:target_status_code:int}|-)
 %{INT:received_bytes:int} %{INT:sent_bytes:int} \"%{ELB_REQUEST_LINE}\" \"(?:%{DATA:user_agent}|-)\"
 (?:%{NOTSPACE:ssl_cipher}|-) (?:%{NOTSPACE:ssl_protocol}|-) （紙面の都合により改行）
  %{NOTSPACE:target_group_arn} \"%{NOTSPACE:trace_id}\"
//}


パターンファイルを準備したので、パイプラインファイルの@<code>{alb.conf}にFilterを追加します。


//list[logstash-30][alb.confの編集]{
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


編集が完了したら、@<code>{/usr/share/logstash/bin/logstash -f /etc/logstash/conf.d/alb.conf}でLogstashを実行します。
最初に実行した時と違い、key-valueの形になっていることがわかります。


//cmd{
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
            "target_group_arn" => "arn:aws:elasticloadbalancing:us-east-2:123456789012:
																	targetgroup/my-targets/73e2d6bc24d8a067",
                     "urihost" => "www.example.com:443",
                        "path" => [
        [0] "/etc/logstash/alb.log",
        [1] "/"
    ]
//}


それでは、Filterで記載している内容について説明します。
今回使用しているフィルタは次の通りです。

 1. grok
 2. date
 3. geoip
 4. mutate
　
　
１．grok-filter

正規表現でデータをパースする際に使用します。
@<code>{patterns dir}で外出ししているパターンファイルを呼び出すことができます。
また、@<code>{match}でmessageに取り込まれている値を対象にGrok-Patterns（ここでいうALB ACCESS_LOG）を適用しています。
　
２．date-filter

実際のログが出力された時間を@<code>{@timestamp}に置き換えています。
そのままではLogstashがデータを取得した時刻が@<code>{@timestamp}に記録されてしまうからです。
今回は@<code>{date}を@<code>{@timestamp}に置き換えています。
また、タイムゾーンを日本にしたいため、"Asia/Tokyo"を指定しています。
　
３．geoip-filter

IPアドレスから地理情報を取得することが可能です。@<fn>{1}
例えば、どこかのグローバルIPアドレスからWhoisでどこの国からのアクセスかな？と調べる時があります。
その動作を一つひとつのログに対して行うのは大変です。しかし、geoip-filterを使用すれば、自動で地理情報を付与してくれるのです。
これはLogstashが内部で保持しているデータベースを照合して地理情報を付与しています。


ここではgeoip-filterを適用するフィールドを指定します。
今回は、クライアントのIPアドレスを元にどこからアクセスされているかを知りたいため、フィールド名の"client_ip"を指定します。
　
４．mutate-filter

不要なフィールドの削除を行うなど、データやログの編集が可能です。
例えば、messageの値は、全てkey-valueで分割されて保存されています。そのため、無駄なリソースを使いたくない場合は、削除するというような運用を行います。
個人的には、保存されたデータでパースが上手くいかず@<code>{_grokparsefailure}が発生した時の場合も踏まえると、残した方が良いのではないかと考えています。@<fn>{2}

mutate-filterの設定を追加したalb.confは次のようになります。

//list[logstash-31][alb.confにmutate-filterを追加]{
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

これでパイプラインファイルの設定ができました。

==== 実行時のエラーが発生した場合

コマンドラインで実行している際に次のようなエラーが発生した場合は、Logstashのプロセスがすでに立ち上がっている可能性があります。


//cmd{
$ /usr/share/logstash/bin/logstash -f conf.d/alb.conf
WARNING: Could not find logstash.yml which is typically located in $LS_HOME/config or /etc/logstash.
You can specify the path using --path.settings. Continuing using the defaults
Could not find log4j2 configuration at path /usr/share/logstash/config/log4j2.properties.
Using default config which logs errors to the console
[INFO ] 2018-xx-xx xx:xx:xx.xxx [main] scaffold - Initializing module {:module_name=>"netflow",
:directory=>"/usr/share/logstash/modules/netflow/configuration"}
[INFO ] 2018-xx-xx xx:xx:xx.xxx [main] scaffold - Initializing module {:module_name=>"fb_apache",
:directory=>"/usr/share/logstash/modules/fb_apache/configuration"}
[WARN ] 2018-xx-xx xx:xx:xx.xxx [LogStash::Runner] multilocal - Ignoring the 'pipelines.yml'
file because modules or command line options are specified
[FATAL] 2018-xx-xx xx:xx:xx.xxx [LogStash::Runner] runner - Logstash could not be started
because there is already another instance using the configured data directory.
If you wish to run multiple instances, you must change the "path.data" setting.
[ERROR] 2018-xx-xx xx:xx:xx.xxx [LogStash::Runner] Logstash - java.lang.IllegalStateException:
 org.jruby.exceptions.RaiseException: (SystemExit) exit
//}


プロセスを強制的にkillすることで、エラーを解消することが可能です。


//cmd{
$ ps -aux | grep logstash
Warning: bad syntax, perhaps a bogus '-'? See /usr/share/doc/procps-3.2.8/FAQ
root     32061  1.7 12.8 4811812 521780 pts/0  Tl   14:12
1:06 /usr/lib/jvm/java/bin/java -Xms2g -Xmx2g -XX:+UseParNewGC -XX:+UseConcMarkSweepGC
-XX:CMSInitiatingOccupancyFraction=75 -XX:+UseCMSInitiatingOccupancyOnly -XX:+DisableExplicitGC
-Djava.awt.headless=true -Dfile.encoding=UTF-8 -XX:+HeapDumpOnOutOfMemoryError -cp
/usr/share/logstash/logstash-core/lib/jars/animal-sniffer-annotations-1.14.jar:
/usr/share/logstash/logstash-core/lib/jars/commons-compiler-3.0.8.jar:
/usr/share/logstash/logstash-core/lib/jars/error_prone_annotations-2.0.18.jar:
/usr/share/logstash/logstash-core/lib/jars/google-java-format-1.5.jar:
/usr/share/logstash/logstash-core/lib/jars/guava-22.0.jar
:/usr/share/logstash/logstash-core/lib/jars/j2objc-annotations-1.1.jar:
/usr/share/logstash/logstash-core/lib/jars/jackson-annotations-2.9.1.jar:
/usr/share/logstash/logstash-core/lib/jars/jackson-core-2.9.1.jar:
/usr/share/logstash/logstash-core/lib/jars/jackson-databind-2.9.1.jar:
/usr/share/logstash/logstash-core/lib/jars/jackson-dataformat-cbor-2.9.1.jar:
/usr/share/logstash/logstash-core/lib/jars/janino-3.0.8.jar:
/usr/share/logstash/logstash-core/lib/jars/javac-shaded-9-dev-r4023-3.jar:
/usr/share/logstash/logstash-core/lib/jars/jruby-complete-9.1.13.0.jar:
/usr/share/logstash/logstash-core/lib/jars/jsr305-1.3.9.jar:
/usr/share/logstash/logstash-core/lib/jars/log4j-api-2.9.1.jar:
/usr/share/logstash/logstash-core/lib/jars/log4j-core-2.9.1.jar:
/usr/share/logstash/logstash-core/lib/jars/log4j-slf4j-impl-2.9.1.jar:
/usr/share/logstash/logstash-core/lib/jars/logstash-core.jar:
/usr/share/logstash/logstash-core/lib/jars/slf4j-api-1.7.25.jar org.logstash.Logstash -f conf.d/alb.conf
root     32231  0.0  0.0 110468  2060 pts/0    S+   15:16   0:00 grep --color=auto logstash
$ kill -9 32061
//}


次は、いよいよInputをS3にして、OutputをElasticsearchにする設定を記述します。


==== InputとOutputを変更する


現在の設定は、Inputをローカルファイル指定しており、Outputが標準出力にしてあります。
ここからは、Inputをs3に変更し、OutputをElasticsearchに変更します。
まずは、Inputから編集します。

　　
１．Inputの編集

@<code>{alb.conf}へS3からデータを取得する設定を行います。

//list[logstash-32][alb.confへs3をInputにする設定を追記]{
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

//table[logstahs-33][S3-input-pluginの解説]{
No.	Item	Content
-----------------
1	region	AWSのリージョンを指定
2	bucket	バケットを指定
3	prefix	バケット配下のディレクトリを指定
4	interval	バケットからログを取り込む間隔を指定(sec)
5	sincedb_path	sincedbファイルの出力先を指定
//}


今回は、AWSのアクセスキーとシークレットキーを指定せず、IAM Role
（@<href>{https://docs.aws.amazon.com/ja_jp/AWSEC2/latest/UserGuide/iam-roles-for-amazon-ec2.html}）をインスタンスに割り当てています。
オプションで指定することも可能ですが、セキュリティ面からIAM Roleで制御しています。
　
２．Outputの編集

最後にOutputを標準出力からElasticsearchに変更します。


//list[logstash-34][OutputをElasticsearchに変更]{
output {
  elasticsearch {
    hosts => [ "localhost:9200" ]
  }
}
//}


以下に各オプションについて説明します。
indexを任意の形で指定することも可能ですが、デフォルトのままとするため、指定はしていません。
デフォルトでは@<code>{logstash-logs-%{+YYYYMMdd}で作成されます。

//table[logstash-35][Elasticsearch-output-pluginの解説]{
No.	Item	Content
-----------------
1	hosts	elasticsearchの宛先を指定
//}


これで完成です！
以下に最終的なパイプラインの設定ファイルを記載します。


//list[logstash-36][alb.confの設定]{
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

==== Logstashサービス起動

@<code>{alb.conf}が全て記述できたので、Logstashをサービスコマンドで起動します。

//list[logstash-37][AWSでLogstashをサービス起動する]{
sudo initctl start logstash
//}


indexが取り込まれているかを確認します。
indexが日付単位で取り込まれていることがわかります。

//list[logstash-38][indexが作成されているか確認]{
### Index confirmation
curl -XGET localhost:9200/_cat/indices/logstash*
//}


ログがElasticsearchに保存されたかも合わせて確認します。
@<code>{curl -XGET localhost:9200/{index\}/{type\}/{id\}}の形式で確認できます。
また、@<code>{?pretty}を使用することでjsonが整形されます。


//cmd{
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


=== Kibanaの環境準備


起動前に、Kibanaのディレクトリ構成を確認してみましょう。


//cmd{
/etc/kibana/
 ┗ kibana.yml
//}

==== kibana.ymlの編集


Kibanaはフロント部分のため、アクセス元を絞ったり、参照するElasticsearchの指定したりするなどが可能です。
今回の設定は、アクセス元の制限はしない設定にします。方法は、IPアドレスによる制限になります。
どこからでもアクセスできるように設定するため、@<code>{0.0.0.0}のデフォルトルート設定とします（絞りたい場合は、厳密にIPアドレスを指定することで制限をかけることが可能です）


//list[logstash-39][/etc/kibana/kibana.yml]{
server.host: 0.0.0.0
//}


これで設定は完了です。
参照先のElasticsearchの指定は、デフォルトのままとします。デフォルトの設定が、ローカルホストを指定しているためです。
もしリモートにElasticsearchがある場合は、以下のコメントアウトを外し、IPアドレスを指定してください。


//list[logstash-40][elasticesearch.ymlの設定]{
#elasticsearch.url: "http://localhost:9200"
//}

==== Kibanaサービス起動

Kibanaを起動し、動作確認をします。

//list[logstash-41][Kibanaの起動]{
service kibana start
//}

==== Kibanaで取り込んだログをビジュアライズ

Kibanaにアクセスするため、ブラウザを起動し、以下のIPアドレスを入力します。
@<code>{Global_IP}については、AWSから払い出されたグローバルIPアドレスを入力してください。

//list[logstash-42][KibanaにアクセスするためのURL]{
http:"Global_IP":5601
//}

詳しい操作方法は@<chapref>{Kibana-visualize}も参照してください。

Kibanaのトップページが開きますので、左ペインの@<code>{Management}をクリックしてください。
また、@<code>{Collapse}をクリックすることで、サイドバーを縮小することができます。


//image[kibana01][Managementへ遷移][scale=0.7]{
//}

@<code>{Index Patterns}をクリックします。

//image[kibana02][Indexの設定][scale=0.7]{
//}


indexパターンを指定せずにElasticsearchに取り込んでいるため、@<code>{logstash-YYYY.MM.DD}のパターンで取り込まれます。
そのため、@<code>{Define index pattern}の欄に@<code>{logstash-*}と入力します。

//image[kibana03][Indexを選択][scale=0.7]{
//}

@<code>{Success! Your index pattern matches 1 index.}と表示されたことを確認し、@<code>{Next step}をクリックします。

//image[kibana04][Indexが選択できたことの確認][scale=0.7]{
//}

@<code>{Time Filter field name}に@<code>{@timestamp}を選択し、@<code>{Create index pattern}をクリックします。

//image[kibana05][Indexの作成][scale=0.7]{
//}

これでindexパターンの登録が完了したので、KibanaからElasticsearchのindexをビジュアライズする準備が整いました。
左ペインの@<code>{Discover}をクリックします。

//image[kibana06][Discover][scale=0.7]{
//}

あれ？@<code>{No results found}と画面に表示されており、取り込んだログがビジュアライズされてないですね。
なぜかと言うと、時刻のデフォルト設定は、@<code>{Last 15 minutes}のため、現在時刻から15分前までの時間がサーチ対象となっているからです。
今回取り込んだログの時刻が@<code>{2016-08-10T23:39:43}のため、該当する時間でサーチをかける必要があります。

//image[kibana07][No results found画面][scale=0.7]{
//}

それでは、検索する時間を変更したいため、@<code>{Last 15 minutes}をクリックします。
クリックすると、@<code>{Time Range}が表示されるので、@<code>{Absolute}をクリックし、以下を入力します。

 * From: 2016-08-11 00:00:00.000
 * To: 2016-08-11 23:59:59.999

//image[kibana08][時間の指定][scale=0.7]{
//}

先ほどの@<code>{No results found}画面ではなく、バーが表示されていることがわかるかと思います。
これで取り込んだログをKibanaから確認することができました。
@<code>{Visualize}で、グラフやを作成したり世界地図などにマッピングしたりすることで好みのダッシュボードが作成できます。

//footnote[1][地理情報の精度を上げたい場合は、有料版のデータをインポートする必要があります]

//footnote[2]["_grokparsefailure"は、grokフィルタでパースできない場合に発生します]
