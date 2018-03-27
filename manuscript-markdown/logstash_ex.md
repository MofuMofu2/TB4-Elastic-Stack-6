# 複数のデータソースを取り扱う

5章では、LogstashでS3にあるALBのログを取得し、Elasticsearchにストアしました。  
ElasticsearchにインデクシングされたデータをKibanaでビジュアライズするところまで実施しました。  
実際の運用を考えると複数のデータソースを取り扱うケースが多くあると思います。本章では、複数のデータソースを取り扱う場合のパイプラインファイルの定義方法について説明します。

## 複数データソースを取り扱うための準備

データソースを二つ取得している環境を想定します。  
例えば、ALBのアクセスろぐとApacheのアクセスログの二つです。ALBのアクセスログは、5章と同様にS3をデータソースとし、Apacheは、ローカルファイルとします。  
以下にディレクトリ構成を記載します。

```bash
### Logstash directory
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
```

パイプラインファイルを"alb_httpd.conf"というファイル名にします。また、Apacheのアクセスログは、"/etc/logstash/log/"配下に"httpd_access.log"を配置している前提とします。  
実際にパイプラインファイルの中身を見ていきたいと思います。

```ruby
### Sample pipeline_file
$ vim /etc/logstash/conf/alb_httpd.conf
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
```

今までのパイプラインファイルより、少し長くなってますね。  
どのような処理がされているかを"Input"、"Filter"、"Output"に分けて説明していきます。

### Input処理内容について

Inputは、データソースの取り込み部分の定義箇所ですね。  
今回は、S3とローカルファイルのため、"S3 input plugin"と"File input pulgin"を使用します。  
"File input pulgin"は、デフォルトインストールされているので、すぐに使えます。

```ruby
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
```

"File"で定義しているオプションについて説明します。  

| No. | Item           | Content                                                  |
|:----|:---------------|:---------------------------------------------------------|
| 1   | path           | ファイルが格納されているパスを指定                       |
| 2   | tags           | 任意のタグを付与                                         |
| 3   | start_position | Logstashの起動時にどこからログファイルを読み込むかを指定 |
| 4   | sincedb_path   | sincedbファイルの出力先を指定                            |


"S3"にも"File"にも共通して、"tags"を定義していることがわかります。この"tags"の値を元に"Filter"のif文で分岐させることができます。  
ここでは、ALBのアクセスログのため、"alb"という値を"tags"に与えます。また、Apacheのアクセスログは、"httpd"という値を与えます。

### Filter処理内容について

"Filter"では、様々なフィルタをかけられることを5章で体験したかと思います。今回は、タグをベースにif文による分岐を行なっています。  
if文の文法は、rubyに則ったかたちです。

```ruby
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
      match => { "message" => "%{HTTPD_COMBINED_LOG}" }
    }
    geoip {
      source => "clientip"
    }
    date {
      match => [ "date", "dd/MMM/YYYY:HH:mm:ss Z" ]
      locale => "en"
      target => "timestamp"
    }
    useragent {
    source => "agent"
    target => "useragent"
    }
    mutate {
      remove_field => [ "path", "host", "date" ]
    }
  }
}
```

ここで新たにApache用のパターンファイルを準備できていないので、作成します。  

```ruby
$ vim /etc/logstash/patterns/httpd_patterns
# Access_log
HTTPDUSER %{EMAILADDRESS}|%{USER}
HTTPD_COMMON_LOG %{IPORHOST:clientip} %{HTTPDUSER:ident} %{HTTPDUSER:auth} \[%{HTTPDATE:timestamp}\] "(?:%{WORD:verb} %{NOTSPACE:request}(?: HTTP/%{NUMBER:httpversion})?|%{DATA:rawrequest})" %{NUMBER:response} (?:%{NUMBER:bytes}|-)
HTTPD_COMBINED_LOG %{HTTPD_COMMONLOG} %{QS:referrer} %{QS:agent}
```

これで、Apacheのアクセスログに対しての"grok-filter"をかけることが可能になりました。  
今回新しい"Filter"で使用しているオプションについて説明します。

#### Useragent filter plugin

"useragent-filter"を使用することで、サイトにアクセスしてきたデバイス情報や、ブラウザのバージョンなどの情報を構造化してくれます。
"grok-filter"をかけると、"agent"のフィールドにユーザエージェントのデータがあるので、このフィールドに対してフィルタをかけています。  
また、元データもとっておくため、"target"指定で別フィールドの"useragent"に出力しています。

```ruby
useragent {
source => "agent"
target => "useragent"
}
```

#### Mutate filter plugin

5章で"mutate-filter"を利用すれば、不要なフィールドの削除ができるという説明をしています。  
実際にフィールド削除を行なう場合は、以下のように記載します。今回は、"path"、"host"、"date"を削除対象としています。

```ruby
mutate {
  remove_field => [ "path", "host", "date" ]
}
```

### Output処理内容について

5章では、インデックスパターンをデフォルト（"logstash-YYYY.MM.DD"）にしていましたが、複数の場合は、個々でインデックスパターンを指定する必要があります。  
理由は、別々の用途で利用するログのため、インデックスを分ける必要があるためです。  
本来は、一つのログしか取り扱わない場合でもインデックスパターンを指定する方がいいです（Logstashというインデックス名だと、どのような用途のインデックスがわかりにくいため）  

```ruby
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
```

"output"でも、if文によるタグ分岐をすることで、出力先を指定することが可能です。  
"alb"タグが付与されているデータは、"alb-logs-%{+YYYYMMdd}"というかたちでインデクシングされます。また、"httpd"タグ付与されている場合は、"httpd-logs-%{+YYYYMMdd}"というかたちでインデクシングされます。  

### 準備が整ったので実行するよ

複数のログファイルを取り込める準備が整ったので、Logstashを再起動します。

```ruby
### Restart logstash service
$ initctl restart logstash
```

ここまでやって如何でしたか？これで複数のログを取り込むことができるようになりました。  
お気付きになった方もいるかと思いますが、更にログが増えた場合に、どんどんif文が増えて、可読性が悪くなっていきます。これは、ツラミでしかないです。  
また、ログの種類によって、ログ量やフィルタの内容も異なるので、リソースは柔軟に対応したいのですが、柔軟にできないです。  
でも、Logstashの6系からできるようになりました！てことで、ここから、イケてる"Multiple Pipelines"について説明していきます。

## Multiple Pipelinesについて

先ほどまで、一つのファイルにパイプラインファイルを記載していました。  
しかし、"Multiple Pipelines"を使用することで、データソース毎にパイプラインファイルを分けて定義することができます。  
また、リソースの配分もデータソースでできます。それでは、具体的にどのような設定をする必要があるかを見ていきます。


### Multiple Pipelinesの定義をしてみる

"Multiple Pipelines"の定義ファイルは、"pipelines.yml"です。  
"pipelines.yml"にパイプラインファイルを指定するだけです。その際に、リソースの指定もできます。
それでは、"pipelines.yml"に、ALBとApacheのアクセスログを取り込むパイプラインファイルを設定したいと思います。

```ruby
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
```

それでは、各オプションについて見ていきたいと思います。

| No. | Item            | Content                                                                                                                                                            |
|:----|:----------------|:-------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1   | pipeline.id     | 任意のパイプラインIDを付与できます                                                                                                                                 |
| 2   | pipeline.batch  | 個々のワーカースレッドのバッチサイズの指定。サイズを大きくすれば効率的に処理が可能だが、メモリオーバヘッドが増加する可能性がある。また、ヒープサイズにも影響する。 |
| 3   | path.config:    | パイプラインファイルのパス指定                                                                                                                                     |
| 4   | pipeline.worker | ワーカーの数を指定                                                                                                                                                 |

[オプションについて](https://www.elastic.co/guide/en/logstash/6.x/logstash-settings-file.html)




前回の章で説明したLogstashのオプションを個別で設定することができます。
そのため、"pipeline.workers"で割り当てるWorker数を指定することが可能です。

logstash.ymlでは、"path.config"の指定を以下のように一律で行なっていました。

> path.config: /etc/logstash/conf.d/*.conf

pipelines.ymlでは、以下のように個別に指定します。
拡張子は、"cfg"とします。

> path.config: /etc/logstash/conf.d/*.cfg

"pipeline.id"は、任意の名前で記載します。

これでMulti Pipelinesの設定は完了です。
あとは、conf.d配下のファイル名をcfgの拡張子に変更するだけです。

実際にALBの設定ファイルを用いて起動までやりたいと思います。

pipelines.ymlを作成します。

```bash
### create pipelines.yml
vim /etc/logstash/pipelines.yml
- pipeline.id: alb
  pipeline.batch.size: 125
  path.config: "/etc/logstash/conf.d/alb.cfg"
  pipeline.workers: 1
```

"alb.conf"のファイル名を"alb.cfg"に変更します。

```bash
### Change extension
mv /etc/logstash/conf.d/alb.conf /etc/logstash/conf.d/alb.cfg 
```

それでは起動します。

```bash
### Start logstash service
$ initctl start logstash
```

起動時にエラーが発生していないかをログで確認します。
"ERROR"が発生していないかを確認し、"Pipelines running"であることを確認します。

```bash
###
$ tail -f /var/log/logstash/logstash-plain.log
[2018-xx-xxTxx:xx:xx,xxx][INFO ][logstash.agent           ] Pipelines running {:count=>1, :pipelines=>["alb"]}
```

これからは、Multiple Pipelinsを使いましょう！





## Multiple Pipelinesについて

5章で作成したパイプラインは、一つのデータソースに対してのみを取り込んでいました。しかし、実際の運用では、たくさんのデータソースを取り扱うかと思います。  
そんな時に、どのような対応を今までとってきかと言うと、タグを付与して、if文で分岐させて処理させるようにしていました。  
具体的にどのような構成になるか見ていきたいと思います。