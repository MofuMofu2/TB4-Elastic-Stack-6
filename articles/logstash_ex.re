
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


このようにconf.dに配置したファイルが対象の設定ファイルと見なされます。
ログによっては、ログ量やフィルターなど負荷のかかり具合が違ってくるのですが、柔軟なリソース供給ができません。
前章で説明したWoker数などを設定ファイル毎に設定することができないのです。



また、一つの設定ファイルにまとめて設定ファイルを作成するケースもあると思います。
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


このように設定ファイルやリソースを柔軟に設定がLogstashの6系から可能になりました。
それでは、Multiple Pipelinesの説明をしていきたいと思います。


== Multiple Pipelines


Multiple Pipelinesを使用するには、"pipelines.yml"に対象のパイプラインの設定ファイルを指定します。
例えば、ALBを対象とした設定ファイルとHTTPDを対象とした設定ファイルがあったとします。
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


前回の章で説明したLogstashのオプションを個別で設定することができます。
そのため、"pipeline.workers"で割り当てるWorker数を指定することが可能です。



logstash.ymlでは、"path.config"の指定を以下のように一律で行っていました。


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

