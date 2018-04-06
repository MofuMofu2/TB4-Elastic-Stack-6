= 複数のデータソースを取り扱う


@<chapref>{logstash}では、LogstashでS3にあるALBのログを取得し、Elasticsearchに送付しました。
ElasticsearchにインデクシングされたデータをKibanaで可視化するところまで実施しました。

しかし、実際に何かのサービスを運用する際は、複数のデータソースを取り扱うケースが多いです。
本章では、複数のデータソースを取り扱う場合のパイプラインファイルの設定方法について説明します。


== 複数データソースを取り扱うための準備

データソースを二つ取得している環境を想定します。
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
@<code>{tags}を定義しいます。@<code>{tags}の値を元に後の処理内容をif分岐させることができます。
ここでは、ALBのアクセスログには@<code>{alb}という@<code>{tags}を設定しました。
また、Apacheのアクセスログは、@<code>{httpd}という@<code>{tags}を設定しています。


=== Filter処理内容について

本章では、Inputで定義した@<code>{tags}をベースにif分岐を用いた処理を行ないました。
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
この処理の前にGrok処理を行なっているので、フィールド@<code>{agent}にユーザエージェントのデータがパースされた状態になります。
このフィールドに対してフィルタをかけています。
また、元データを保持したいので、@<code>{target}オプションで元データを別フィールドの@<code>{useragent}に出力しています。

==== Mutate filter plugin


5章で"mutate-filter"を利用すれば、不要なフィールドの削除ができるという説明をしています。
実際にフィールド削除を行なう場合は、以下のように記載します。今回は、"path"、"host"、"date"を削除対象としています。


//list[][ruby]{
mutate {
  remove_field => [ "path", "host", "date" ]
}
//}

=== Output処理内容について


5章では、インデックスをデフォルト（"logstash-YYYY.MM.DD"）にしていましたが、複数の場合は、個々でインデックスを指定する必要があります。
理由は、別々の用途で利用するログのため、インデックスを分ける必要があるためです。
本来は、一つのログしか取り扱わない場合でもインデックスを指定する方がいいです（Logstashというインデックス名だと、どのような用途のインデックスがわかりにくいため）


//list[][ruby]{
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
//}


"output"でも、if文によるタグ分岐をすることで、出力先を指定することが可能です。
"alb"タグが付与されているデータは、"alb-logs-%{+YYYYMMdd}"でインデクシングされます。また、"httpd"タグが付与されている場合は、"httpd-logs-%{+YYYYMMdd}"でインデクシングされます。


=== 準備が整ったので実行するよ


複数のログファイルを取り込める準備が整ったので、Logstashを再起動します。


//list[][ruby]{
### Restart logstash service
$ initctl restart logstash
//}


ここまでやって如何でしたか？これで複数のログを取り込むことができるようになりました。
お気付きになった方もいるかと思いますが、更にログが増えた場合に、どんどんif文が増えて、可読性が悪くなっていきます。これは、ツラミでしかないです。
また、ログは、種類によってログ量やフィルタの内容も異なってきます。そのため、使用するリソースもバラバラとなります。しかし、5系までのLogstashでは、柔軟にリソースの振り分けを行うことができませんでした。
でも大丈夫です！Logstashの6系からできるようになりました！
てことで、ここから、イケてる"Multiple Pipelines"について説明していきます。


== Multiple Pipelinesについて


先程まで、一つのファイルにパイプラインファイルを記載していました。
しかし、"Multiple Pipelines"を使用することで、データソース毎にパイプラインファイルを分けて定義することができます。
また、リソースの配分もログの種類毎にできます。それでは、具体的にどのような設定をする必要があるかを見ていきます。


=== Multiple Pipelinesの定義をしてみる


"Multiple Pipelines"の定義ファイルは、"pipelines.yml"です。
"pipelines.yml"にパイプラインファイルを指定するだけです。その際に、リソースの指定もできます。
それでは、"pipelines.yml"に、ALBとApacheのアクセスログを取り込むパイプラインファイルを設定したいと思います。


//list[][ruby]{
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


それでは、各オプションについて見ていきたいと思います。

//table[tbl2][]{
No.	Item	Content
-----------------
1	pipeline.id	任意のパイプラインIDを付与できます
2	pipeline.batch	個々のワーカースレッドのバッチサイズの指定。サイズを大きくすれば効率的に処理が可能だが、メモリオーバヘッドが増加する可能性がある。また、ヒープサイズにも影響する。
3	path.config:	パイプラインファイルのパス指定
4	pipeline.worker	ワーカーの数を指定
//}


@<href>{https://www.elastic.co/guide/en/logstash/6.x/logstash-settings-file.html,オプションについて}



これで"Multiple Pipelines"の定義は完了です。
ただ、これでは動かないので"Multiple Pipelines"への対応として、パイプラインファイルの分割とファイル名（拡張子）を変更します。


=== パイプラインファイルの分割


パイプラインファイルの"alb_httpd.conf"を分割します。
まず、ALB用のパイプラインファイルとして、"alb.cfg"にします。
拡張子を"conf"から"cfg"に変更します。拡張子を"conf"のままでも問題ないのですが、公式ドキュメントに則って"cfg"にします。



以下のように"alb.cfg"を作成します。
特に中身は変わらず、"httpd"の部分とif文を削除しただけですね。


//list[][ruby]{
### pipeline_file
$ vim /etc/logstash/conf/alb.cfg
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


あわせて"httpd.cfg"も作成します。


//list[][ruby]{
### pipeline_file
$ vim /etc/logstash/conf/httpd.cfg
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


これで分割とファイル名の変更が完了しましたので、Logstashを再起動します。


//list[][ruby]{
### Restart logstash service
$ initctl restart logstash
//}

=== ログの確認


Logstashがうまく動いてくれないなどがある場合は、ログを見ましょう。
ログは、"/var/log/logstash/"配下に出力されます。したがいまして、Logstashを起動した時に"tail"で起動がうまくいっているかの確認をすると良いです。


//list[][bash]{
### Check Log
$ tail -f /var/log/logstash/logstash-plain.log
[2018-xx-xxTxx:xx:xx,xxx][INFO ][logstash.agent           ] Pipelines running {:count=>1, :pipelines=>["alb"]}
//}


いかがでしたか？
ここまで動かせたらLogstashをかなり理解できたのではと思います。
次の章では、Logstashより簡易にログを取り込んで、ビジュアライズまでやりたいというニーズに応えることができるBeatsというプロダクトを説明していきます。
