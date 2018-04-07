= 複数のデータソースを取り扱う


@<chapref>{logstash}では、LogstashでS3にあるALBのログを取得し、Elasticsearchに送付しました。
そして、ElasticsearchにインデクシングされたデータをKibanaで可視化するところまで実施しました。

しかし、実際に何かのサービスを運用する際は、複数のデータソースを取り扱うケースが多いです。
本章では、複数のデータソースを取り扱う場合のパイプラインファイルの設定方法について説明します。


== 複数データソースを取り扱うための準備

データソースを2つ取得している環境を想定します。
ALBのアクセスログとApacheのアクセスログの2つを取得するケースです。
ALBのアクセスログは、@<chapref>{logstash}と同様にS3をデータソースとし、Apacheのアクセスログ@<code>{httpd_access.log}は
ローカルのディレクトリに配置したものを取得します。
以下にディレクトリ構成を記載します。


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


今までのパイプラインファイルより、少し複雑になっています。
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

@<list>{logstash_pipelines-02}では、Inputで定義した@<code>{tags}をベースにif分岐を用いた処理を行いました。
if文の記述方法はRubyの記法で記述します。

ここでもGrok処理を行なっているのですが、Apache用のパターンファイルを準備できていないので@<code>{httpd_patterns}を作成します。


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


@<chapref>{logstash}で@<code>{mutate-filter}を利用すれば、不要なフィールドの削除ができるという説明をしています。
実際にフィールド削除を行う場合は、以下のように記載します。今回は、@<code>{path}、@<code>{host}、@<code>{date}を削除対象としています。


=== Output処理内容について


5章では、インデックスをデフォルト（@<code>{logstash-YYYY.MM.DD}）にしていましたが、複数の場合は、個々でインデックスを指定する必要があります。
ログの用途が異なるため、インデックスを分けた方が管理がしやすいです。
本来は1つのログしか取り扱わない場合でもインデックスを指定したほうが良いです。インデックス名で用途がすぐ把握できる方が管理しやすいためです。

Outputの中でもif文処理の記述が可能です。今回はif分岐を利用してログの出力先インデックスを分けています。
@<code>{tags}に@<code>{alb}が付与されているデータは、@<code>{alb-logs-%{+YYYYMMdd}というインデックスへデータを転送します。
また、@<code>{tags}に@<code>{httpd}タグが付与されている場合は、@<code>{httpd-logs-%{+YYYYMMdd}というインデックスへデータを転送します。


=== 準備が整ったので実行するよ

複数のログファイルを取得する準備が整ったので、Logstashを再起動します。

//list[logstash_pipelines-04][Logstashの再起動]{
sudo initctl restart logstash
//}


これで複数のログを取り込むことができるようになりました。

お気付きになった方もいるかと思いますが、このままだと取得したいログが増えると、どんどんif文が増えてコンフィグの可読性が悪くなっていきます。
これはツラミでしかないです。

またログの種類によっても、ログ量やフィルタの内容は異なります。
そのため、Logstashが処理に必要とするリソースもログごとに異なってきます。
しかし、5系までのLogstashではデータの処理に利用するリソースの振り分けを行うことができませんでした。

でも大丈夫です！Logstashの6系からはデータごとにLogstashが利用するリソースを配分することが可能となりました！
ここからは、今1番イケてる@<code>{Multiple Pipelines}について説明していきます。


== Multiple Pipelinesについて


先程まで、一つのファイルにパイプラインファイルを記載していました。
しかし、"Multiple Pipelines"を使用することで、データソース毎にパイプラインファイルを分けて定義することができます。
また、リソースの配分もログの種類毎にできます。それでは、具体的にどのような設定をする必要があるかを見ていきます。


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
2	pipeline.batch	個々のワーカースレッドのバッチサイズの指定。サイズを大きくすれば効率的に処理が可能だが、メモリオーバヘッドが増加する可能性がある。また、ヒープサイズにも影響する。
3	path.config:	パイプラインファイルのパス指定
4	pipeline.worker	ワーカーの数を指定
//}

これでMultiple Pipelinesの定義は完了です。
ただ、これではLogstashは動作しません。Multiple Pipelinesへの対応として、パイプラインファイルの分割とファイル名（拡張子）を変更します。


=== パイプラインファイルの分割


パイプラインファイルの@<code>{alb_httpd.conf}を分割します。
まず、ALB用のパイプラインファイルとして、@<code>{alb.cfg}にします。
拡張子を@<code>{conf}から@<code>{cfg}に変更します。拡張子@<code>{conf}のままでも問題ないのですが、
ここでは公式ドキュメントに則って@<code>{cfg}にします。

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
