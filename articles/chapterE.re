= 比較その3：色々な観点から

簡単ではありますが、fluentdとLogstashを動作させることができました。
なので、様々な観点から2つのソフトウェアを比較してみます。


//table[difference_software][fluentdとLogstashの違いまとめ]{
比較観点	fluentd	Logstash
------------------------------------------
気楽に始められる度	◯	△
ドキュメントの親切度	◯	△
ログのわかりやすさ	◯	△
Windowsとの相性	△	◯
Beatsとの相性	△	◯
改行があるデータを扱えるか	◯	◯
インターネット必須度	◯	◯
アップデートの速さ	△	◯
//}

== 気楽に始められる度
インストールと動作確認に準備がほとんど必要ないので、fluentdの方がとっつきやすく簡単に始められます。
Logstashは事前準備としてJavaのインストールやリポジトリ登録が必要ですし、
動作確認用にコンフィグを新たに作成する必要があります。

対するfluentdはインストールはスクリプトが全部やってくれますし、何より
動作確認用のコンフィグがあらかじめ準備されています。コンフィグのお手本があるのはとても嬉しいですね。

== ドキュメントの親切度
これは完全なる独断と偏見ですが、fluentdの方が圧倒的にわかりやすいです。
プラグインごとに、実装例と動作例が細かく記載されているからです。
オプションも使える選択肢がまとまって記載されているので、1ページをしっかり読めばプラグインの使い方がわかるようになっています。

Logstashのドキュメントは実装例は少し記載があるものの、どのように動作するのかは文章でしか記載されていません。
コンフィグの書き方も具体例が少ないので、他人の実装例と一緒にドキュメントを読んだ方が良いかもしれません。

//footnote[Logstash_doc][あと、outputのcsvプラグイン、実装例が間違ってる気がする…。気のせい？]

== ログのわかりやすさ
これも完全なる独断と偏見ですが、fluendの方が圧倒的にわかりやすいです。
同じ原因のエラーログを並べて比較してみます。
どちらもcsvファイルの列となる文字列を設定し忘れたときのエラーです。これも紙面の都合上、適宜改行を入れています。

//list[fluentd_error][fluentdのエラーログ]{
Starting td-agent: 2017-10-08 10:15:48 +0900
[error]: fluent/supervisor.rb:373:rescue in main_process:
config error file="/etc/td-agent/td-agent.conf"
error="'fields' parameter is required"
td-agent                                                   [FAILED]
//}

原因が簡潔にまとまっていてわかりやすいです。

//list[Logstash_error][Logstashのエラーログ]{
[2017-10-01T13:12:22,348][INFO ][logstash.modules.scaffold]
Initializing module {:module_name=>"fb_apache", :
directory=>"/usr/share/logstash/modules/fb_apache/configuration"}
[2017-10-01T13:12:22,352][INFO ][logstash.modules.scaffold]
Initializing module {:module_name=>"netflow",
:directory=>"/usr/share/logstash/modules/netflow/configuration"}
[2017-10-01T13:12:22,578][WARN ][logstash.config.source.multilocal]
Ignoring the 'pipelines.yml'
file because modules or command line options are specified
[2017-10-01T13:12:22,962][INFO ][logstash.agent           ]
Successfully started Logstash API endpoint {:port=>9600}
[2017-10-01T13:12:23,399][ERROR][logstash.outputs.csv     ]
Missing a required setting for the csv output plugin:

  output {
    csv {
      fields => # SETTING MISSING
      ...
    }
  }
[2017-10-01T13:12:23,401][ERROR][logstash.agent           ]
Failed to execute action
{:action=>LogStash::PipelineAction::Create/pipeline_id:main,
:exception=>"LogStash::ConfigurationError",
:message=>"Something is wrong with your configuration."}
# この1セットが延々と出力される
//}

Logstashはデータの読み取りごとにエラーログが出力されます。
だいたい2000行くらいログが一度に出てくるので、ログを読むのは意外と大変です…。
@<code>{ERROR}の文字列でgrepをかけることをおすすめします。

fluentdはサービス起動時に何かしらのエラーがあるとエラーログを標準出力して動作を停止しますが、Logstashはエラーがあっても一旦サービスが起動します。
その後@<code>{logstash.log}にエラーを出力してサービスを停止します。
エラー時の動作は、やはりfluentdの方が親切だと思います。

また、fluentdはinfoのログも親切です。

//list[fluentd_info][td-agent.logの抜粋]{
2017-10-08 13:43:27 +0900 [info]:
reading config file path="/etc/td-agent/td-agent.conf"
# 省略
2017-10-08 13:43:27 +0900 [info]:
using configuration file: <ROOT>
  <match>
    @type file
    path /var/log/csv/test.csv
    format csv
    fields Full Name,Country,Created At,Id,Email
    buffer_path /var/log/csv/test.csv.*
  </match>
  <source>
    @type tail
    path /var/log/json/*.json
    pos_file /var/log/td-agent/tmp/json.log.pos
    format json
    tag json
  </source>
</ROOT>
2017-10-08 13:43:27 +0900 [info]:
following tail of /var/log/json/test.json
2017-10-08 13:43:55 +0900 [info]:
detected rotation of /var/log/json/test.json; waiting 5 seconds
2017-10-08 13:43:55 +0900 [info]:
following tail of /var/log/json/test.json
//}

自分のコンフィグの設定・データの読み取り対象などが動作ログとして出力されます。
ログを見るだけで、動作がある程度わかるのは便利ですね。これはfluentdにしかない機能です。

== Windowsとの相性
これはLogstashに軍配が上がります。

LogstashはWindows用のZipファイルが提供されており、Linux版との機能差もありません。
対するfluentdですが、現状ではWindowsに対応していないため@<fn>{fluentd_windows}、一旦Linuxサーバーにデータを転送して処理するなどの
工夫が必要です。Windowsでデータ収集・加工を行うのであれば、おとなしくLogstashを使った方が良いです。

//footnote[fluentd_windows][バージョン0.14では対応していますが、beta版なのでここでは対象外です。]

== Beatsとの相性
Elastic社が提供する軽量データジッパーのBeats（@<href>{https://www.elastic.co/jp/products/beats}）というプロダクトがあります。
サーバーのメトリクス情報やネットワークパケットなどを収集し、簡単に整形して転送するツールです。
ただし、Beatsはデータの複雑な加工はできません。なので、データを加工したい場合はfluentdやLogstashと組み合わせて使うことになります。

ここはやはり、同じElastic社製のLogstashと組み合わせて使う事をおすすめします。
というのも、BeatsはElastic社独自の@<code>{lumberjack}プロトコルを使用して通信を行います。
LogstashはBeatsのinputプラグインを使用すれば、問題なくBeatsからデータを取得できます。

fluentdもプラグインを導入すればBeatsと通信できます（@<href>{https://github.com/repeatedly/fluent-plugin-beats}）が、
Logstashと比べると情報が少ないのが現状です。
やはり公式サポートが受けられる、Logstashと組み合わせて使った方が安心かと思います。

== 改行があるデータを扱えるか
改行があるデータとは、次のようなデータのことです。

//list[data_multiline][改行があるデータ]{
{
  "Full Name": "Ms. Florian Bashirian",
  "Country": "U.S. Minor Outlying Islands",
  "Created At": "1983-02-20T15:49:44.163Z",
  "Id": 0,
  "Email": "Delia_Upton@concepcion.name"
}
//}

本当は中括弧で1セットのjsonデータですが、fluentdやLogstashでは改行で1つのデータを認識します。
なので@<list>{data_multiline}のようなデータを正しく認識させるためには工夫が必要です。

fluentdの場合、@<code>{multiline}プラグイン（@<href>{https://docs.fluentd.org/v0.12/articles/parser_multiline}）を使用することで、
複数行のデータを取得することができるようになります。

//list[fluentd_multiline][mltilineプラグイン]{
<source>
# 記述省略
format multiline
</source>
//}

Logstashの場合、inputプラグインのオプションで@<code>{codec => multiline}
（@<href>{https://www.elastic.co/guide/en/logstash/6.x/plugins-codecs-multiline.html}）を指定すると
複数行のデータを取得できます。@<code>{what}は@<code>{pattern}で設定した表現の前か後、どちらをデータとして読み取るか決定するための設定です。


//list[Logstash_mltiline][codec => mltiline]{
input {
  何かしらのプラグイン {
    codec => multiline {
      pattern => "正規表現でデータの型を記載"
      what => "previous/next"
    }
  }
}
//}

これらの情報から、fluentd・Logstash共に複数行のデータは取得できるため機能差はないことがわかります。
ただし、データを正規表現に直して記載するのは大変です。できればデータの出力時点で改行がないようにしておきたいものです。


== インターネット必須度
どちらもインターネット接続ができない環境では運用するのが難しいです。
fluentdはインターネット経由で専用スクリプトを使ってインストールを行いますし、
デフォルトにはないプラグインを利用するにはインターネット越しにダウンロードする必要があります。

対するLogstashですが、zipファイルを使用すればインターネット無しでもインストールはできます。
ただし、こちらもデフォルトにはないプラグインを使用する場合、インターネット経由でプラグインをインストールしなければなりません。

なので、インターネットに繋げないような環境でデータを収集する場合、おとなしく別の方法を考えた方がいいと思います。

ちなみにインストールされているプラグインは、fluentd・Logstash共にコマンドで確認できます。

//list[fluentd_plugin_lists][fluentdで現在インストールされているプラグインを確認する]{
/opt/td-agent/embedded/bin/fluent-gem list
//}

コマンドを実行すると、次のように出力されます。

//cmd{
$ /opt/td-agent/embedded/bin/fluent-gem list

*** LOCAL GEMS ***

actionmailer (4.2.8)
actionpack (4.2.8)
actionview (4.2.8)
activejob (4.2.8)
# 以下省略
//}

//list[Logstash_plugin_lists][Logstashで現在インストールされているプラグインを確認する]{
/usr/share/logstash/bin/logstash-plugin list
//}

コマンドを実行すると、次のように出力されます。バージョンを知りたい場合、@<code>{--verbose}オプションを使用します。

//cmd{
[root@test-logstash ~]# /usr/share/logstash/bin/logstash-plugin list
logstash-codec-cef
logstash-codec-collectd
logstash-codec-dots
logstash-codec-edn
# 以下省略
//}


== アップデートの速さ
圧倒的にLogstashの方が速いです。なんと1年で1つメジャーバージョンが上がります。
そして機能が大幅に追加・変更になったりすることが多いです。
2016年の10月にバージョン5.0.0が出たのですが、2017年10月時点でバージョン5.6になってます。
Beta版で既にバージョン6が公開されているので、来年までにメジャーバージョンが上がること間違いなしです。
ちなみに、古くなりすぎたバージョンのドキュメントは閲覧不可能になるので注意が必要です。
なので、アップデートをかけにくい環境ではLogstashを使うのは厳しいかもしれません。

対するfluentdは2014年にバージョン0.12がリリースされ、こちらが最新版です（0.14もリリースされていますが、Beta版です）。
マイナーバージョンのアップデートはありますが、それでもLogstashのアップデートに比べれば遅いですね。

//indepimage[img01_e][換羽期を迎えてもふもふになるペンギン]
