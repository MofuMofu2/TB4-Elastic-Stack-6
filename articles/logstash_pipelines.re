﻿= 複数のデータソースを取り扱う

今までは一箇所のファイルからデータを取得していました。
しかし、実際に何かのサービスを運用する際は、複数のデータソースを取り扱うケースが多いでしょう。
本章では、複数のデータソースを取り扱う場合のパイプラインファイルの設定方法について説明します。


== 複数データソースを取り扱うための準備

まず、データソースを2つ取得している環境を想定します。
ここではALBのアクセスログとApacheのアクセスログの2つを取得するケースです。

ALBのアクセスログは、「AWSでLogstashを使ってみる」と同様にS3をデータソースとし、Apacheのアクセスログ@<code>{httpd_access.log}は
ローカルのディレクトリに配置したものを取得します。

次にディレクトリ構成を記載します。


//cmd{
/etc/logstash/
 ┣ conf.d
 ┃ ┗ alb_httpd.conf
 ┣ jvm.options
 ┣ log
 ┃ ┗ httpd_access.log
 ┣ log4j2.properties
 ┣ logstash.yml
 ┣ patterns
 ┃ ┣ alb_patterns
 ┃ ┗ httpd_patterns
 ┣ pipelines.yml
 ┗ startup.options
//}


パイプラインファイルを@<code>{alb_httpd.conf}というファイル名にします。
また、Apacheのアクセスログは、@<code>{/etc/logstash/log/}配下に@<code>{httpd_access.log}を配置している前提とします。

//list[logstash_pipelines-01][alb_httpd.confの編集]{
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
      patterns_dir => ["/etc/logstash/patterns/alb_patterns"]
      match => { "message" => "%{ALB_ACCESS}" }
      add_field => { "date" => "%{date01} %{time}" }
    }
    date {
      match => [ "date", "ISO8601" ]
      timezone => "Asia/Tokyo"
      target => "@timestamp"
    }
    geoip {
      source => "client_ip"
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
      index => "alb-logs-%{+YYYYMMdd}"
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


これまでのpipelene fileより、少し複雑になっています。
どのような処理がされているかをInput、Filter、Outputに分けて説明します。


=== Inputの処理内容

Inputは、データソースの取り込み部分の定義箇所ですね。
今回は、S3とローカルファイルのため、@<code>{S3 input plugin}と@<code>{File input pulgin}を使用します。
@<code>{File input plugin}で利用しているオプションの詳細を@<table>{logstash_pipelines-02}にまとめました。

//table[logstash_pipelines-02][File input pluginのオプション]{
No.	Item	Content
-----------------
1	path	ファイルが格納されているパスを指定
2	tags	任意のタグを付与
3	start_position	Logstashの起動時にどこからログファイルを読み込むかを指定
4	sincedb_path	sincedbファイルの出力先を指定
//}


また、@<code>{S3 input plugin}、@<code>{File input plugin}の設定でどちらも
@<code>{tags}を定義しています。@<code>{tags}の値を元に後の処理内容をif分岐させることができます。
ここでは、ALBのアクセスログには@<code>{alb}という@<code>{tags}を設定しました。
また、Apacheのアクセスログは、@<code>{httpd}という@<code>{tags}を設定しています。


=== Filter処理内容について

@<list>{logstash_pipelines-01}では、Inputで定義した@<code>{tags}をベースにif分岐を用いた処理を行いました。
if文の記述方法はRubyの記法で記述します。

ここでもGrok処理を行っているのですが、Apache用のパターンファイルを準備できていないので@<code>{httpd_patterns}を作成します。


//list[logstash_pipelines-03][Apacheのアクセスログ用パターンファイル]{
HTTPDUSER %{EMAILADDRESS}|%{USER}
HTTPD_COMMON_LOG %{IPORHOST:clientip} %{HTTPDUSER:ident} （紙面の都合により改行）
%{HTTPDUSER:auth} \[%{HTTPDATE:timestamp}\] "(?:%{WORD:verb} （紙面の都合により改行）
%{NOTSPACE:request}(?: HTTP/%{NUMBER:httpversion})?|%{DATA:rawrequest})" （紙面の都合により改行）
%{NUMBER:response} (?:%{NUMBER:bytes}|-)
HTTPD_COMBINED_LOG %{HTTPD_COMMONLOG} %{QS:referrer} %{QS:agent}
//}

これでGrok処理を実施するための準備ができました。

==== Useragent filter plugin

@<code>{Useragent file plugin}を利用すると、Webサイトにアクセスしてきたデバイスの情報や、
アクセス時に利用していたブラウザのバージョンなどの情報を構造化できます。
この処理の前にGrok処理を行っているので、フィールド@<code>{agent}にユーザエージェントのデータがパースされて保存されます。
フィールド@<code>{agent}に対してFilter処理を行います。
また、元データを保持するために、@<code>{target}オプションで元データを別フィールドの@<code>{useragent}に出力しています。

==== Mutate filter plugin

「AWSでLogstashを使ってみる」で@<code>{mutate-filter}を利用すれば、不要なフィールドの削除ができるという説明をしています。今回は、@<code>{path}、@<code>{host}、@<code>{date}を削除対象としています。


=== Output処理内容について


5章では、インデックスをデフォルト（@<code>{logstash-YYYY.MM.DD}）にしていましたが、複数の場合は、個々でインデックスを指定する必要があります。
ログの用途が異なるため、インデックスを分けた方が管理がしやすいためです。
本来は1つのログしか取り扱わない場合でもインデックスを指定したほうがよいでしょう。インデックス名で用途がすぐ把握できる方が管理しやすくなります。

Outputの中でもif文処理の記述が可能です。今回はif分岐を利用してログの出力先インデックスを分けています。
@<code>{tags}に@<code>{alb}が付与されているデータは、@<code>{alb-logs-%{+YYYYMMdd}というインデックスへデータを転送します。
また、@<code>{tags}に@<code>{httpd}タグが付与されている場合は、@<code>{httpd-logs-%{+YYYYMMdd}というインデックスへデータを転送します。


=== pipeline fileの実行

複数のログファイルを取得する準備が整ったので、Logstashを再起動します。

//list[logstash_pipelines-04][Logstashの再起動]{
sudo initctl restart logstash
//}

これで複数のログを取り込むことができるようになりました。

このまま取得したいログが増えると、どんどんif文が増えてコンフィグの可読性が悪くなってしまいます。

コンフィグを分けることができれば、この問題は解決できそうです。

== Multiple Pipelinesについて


Logstashのバージョン5以前では、データの処理に利用するリソースの振り分けを行うことができませんでした。

しかし、"Multiple Pipelines"を使用することで、データソース毎にパイプラインファイルを分けて定義することができます。
また、リソースの配分もログの種類ごとにできます。

それでは、具体的にどのような設定をするのかみていきましょう。


=== Multiple Pipelinesの定義をしてみる


Multiple Pipelinesの設定をするために利用する定義ファイルは@<code>{pipelines.yml}です。
@<code>{pipelines.yml}にパイプラインファイルを指定するだけでリソースの指定ができるのです。

それでは、@<code>{pipelines.yml}に、ALBとApacheのアクセスログを取り込むパイプラインファイルを設定したいと思います。


//list[logstash_pipelines-05][/etc/logstash/pipelines.yml]{
- pipeline.id: alb
  pipeline.batch.size: 125
  path.config: "/etc/logstash/conf.d/alb.cfg"
  pipeline.workers: 1
- pipeline.id: httpd
  pipeline.batch.size: 125
  path.config: "/etc/logstash/conf.d/httpd.cfg"
  pipeline.workers: 1
//}

設定したオプションについての説明を@<table>{logstash_pipelines-06}に記載します。
公式ドキュメント（@<href>{https://www.elastic.co/guide/en/logstash/6.x/logstash-settings-file.html}）も
合わせて参考にしてみてください。


//table[logstash_pipelines-06][pipelines.ymlの設定]{
No.	Item	Content
-----------------
1	pipeline.id	任意のパイプラインIDを付与できます
2	pipeline.batch	個々のワーカースレッドのバッチサイズの指定。
3	path.config:	パイプラインファイルのパス指定
4	pipeline.worker	ワーカーの数を指定
//}

@<code>{pipeline.batch}ですが、サイズを大きくすれば効率的に処理が可能です。
しかし、メモリーオーバヘッドが増加する可能性があります。また、ヒープサイズにも影響があるため慎重に設定しましょう。

これでMultiple Pipelinesの定義は完了です。
ただ、これではLogstashは動作しません。Multiple Pipelinesへの対応として、パイプラインファイルの分割とファイル名（拡張子）を変更します。


=== パイプラインファイルの分割


ALBログとApacheのアクセスログ、それぞれの処理用にパイプラインを作成したいので、今ある@<code>{alb_httpd.conf}を分割して設定ファイルを作成します。

まず、ALB用のパイプラインファイルとして、@<code>{alb.cfg}を作成し、@<code>{alb_httpd.conf}の該当部分をコピーします。
このとき拡張子を@<code>{conf}から@<code>{cfg}に変更します。拡張子@<code>{conf}のままでも問題ないのですが、
ここでは公式ドキュメントに則って@<code>{cfg}に設定します。

@<list>{logstash_pipelines-07}は@<code>{alb_httpd.cfg}の内容です。
特に中身に変更はありません。@<code>{httpd}の部分とif文を削除しただけですね。


//list[logstash_pipelines-07][/etc/logstash/conf/alb.cfg]{
input {
  s3 {
    tags => "alb"
    bucket => "hoge"
    region => "ap-northeast-1"
    prefix => "hoge/"
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
    index => "alb-logs-%{+YYYYMMdd}"
  }
}
//}


同様に、@<code>{httpd.cfg}も作成します。


//list[logstash_pipelines-09][/etc/logstash/conf/httpd.cfg]{
input {
  file {
    path => "/etc/logstash/log/httpd_access.log"
    tags => "httpd"
    start_position => "beginning"
    sincedb_path => "/var/lib/logstash/sincedb_httpd"
  }
}
filter {
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
output {
  elasticsearch {
    hosts => [ "localhost:9200" ]
    index => "httpd-logs-%{+YYYYMMdd}"
  }
}
//}


これで分割とファイル名の変更が完了しました。
Logstashを再起動し、設定ファイルの反映をおこないます。


//list[logstash_pipelines-10][Logstashの再起動]{
sudo initctl restart logstash
//}

=== ログの確認


Logstashがうまく動かない場合、まずログを見ましょう。
Logstashの動作ログは、@<code>{/var/log/logstash/}配下に出力されます。

Logstashを起動し、ログファイルを@<code>{tail}コマンドなどで確認しつつ、原因を突き止めていきましょう。

//cmd{
### Check Log
$ tail -f /var/log/logstash/logstash-plain.log
[2018-xx-xxTxx:xx:xx,xxx][INFO ][logstash.agent           ] Pipelines running {:count=>1, :pipelines=>["alb"]}
//}


いかがでしたか？
ここまで動かせたらLogstashをかなり理解できているはずです。
次は、Logstashより簡易にログを取り込みビジュアライズまでやりたい、というニーズに応えることができるBeatsというプロダクトについて説明していきます。
