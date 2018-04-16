= Logstashに触れてみる
== 環境について
Logstash6.0betaがすでにインストールされていることを前提とします。
インストール方法は第4章を参考にして頂ければと思います！@<fn>{stage03_fn01}

//footnote[stage03_fn01][決してサボってるわけじゃないですよ！ページの有効活用]

== 動かす前のLogstash準備
早速ですが、Logstashを動かしたいと思います！

Logstashを動かすには、@<code>{logstash.conf}（以降confファイルとします）という設定ファイルを読み込ませる必要があります。
このconfファイルにINPUT・FILTER・OUTPUTを定義すると、Logstashが処理を実行します。


=== Logstashのディレクトリ構造
Logstashの設定ファイルは@<code>{/etc/logstash}に集約されています。

@<list>{stage03_list01}で
ディレクトリ構造と配置されているファイルの内容について記載しています。今回はrpmパッケージを使ってLogstashをインストールしてます。

//list[stage03_list01][/etc/logstashのディレクトリ構造]{
/etc/logstash/
├ conf.d　(Logstashに実行させたいINPUT・FILTER・OUTPUTをディレクトリ配下に配置する)
├ jvm.options (ヒープサイズの割り当てなどを定義する)
├ log4j2.properties (ロギング設定)
├ logstash.yml (Logstashの設定ファイル)
└ startup.options (Logstash起動設定)
//}



=== confファイルの配置場所について
Logstashは、@<code>{logstash.yml}64行目に記載されているファイルをconfファイルとして読み込みます。
@<list>{stage03_list02}をみてわかるとおり、@<code>{*.conf}となっているため、作成するconfファイルの拡張子は@<code>{.conf}とします。
この設定は変更する理由が特にないので、デフォルト設定のままとします。

//list[stage03_list02][logstash.ymlの64行目]{
path.config: /etc/logstash/conf.d/*.conf
//}


=== confファイルの準備
Logstashを動かす前に簡単なconfファイルを作成します。
confファイルの名前は、test01.confとします。
また、confファイルの配置先は@<code>{/etc/logstash/}です。

//list[stage03_list03][test01.conf]{
input {
  stdin {}
}
output {
  stdout { codec => rubydebug }
}

//}


== 動かすよ！Logstash
Logstashの起動スクリプトは@<code>{/usr/share/logstash/bin/}に配置されています。

//list[stage03_list04][Logstashの起動スクリプト]{
/usr/share/logstash/bin/logstash
//}

Logstashをサービス起動で実行させることもできます。
しかし今回はテストとして動かしたいため、今回は起動スクリプトを使ってLogstashを起動します。
オプションとして@<code>{-f}をつけて、引数にconfファイルを指定します。

では、早速実行してみます。

//cmd{
$ /usr/share/logstash/bin/logstash -f conf.d/test01.conf

# 実行すると入力を待ってるぜ！って言われるので
The stdin plugin is now waiting for input:

# Helloと入力
Hello
{
      "@version" => "1",
          "host" => "0.0.0.0",
    "@timestamp" => 2017-10-01T04:49:23.282Z,
       "message" => "Hello"
}
//}

@<code>{message}という箇所にHelloの文字が入っていますね！
この@<code>{message}の部分は@<code>{field（フィールド）}といいます。
これでLogstashの環境が整いましたーヽ(*ﾟдﾟ)ノ


== Apacheのアクセスログを取り込む
#@#ページ上の方みて欲しいんですが、章と節の見出しが被ってしまったのでタイトル変えました…すまぬ。

それでは早速ですが、ApacheのアクセスログをLogstashで取り込んで、ごにょごにょしてみたいと思いますー
Nginxでもいいんですけど、Google Trendsで"Apache VS Nginx"やってみたら、Apacheに軍配が上がったので、Apacheにしました。

今回は@<list>{stage03_list05}アクセスログで試していきたいと思いますー
Apacheのログフォーマットは、@<code>{common}とします。

#@#Apacheのアクセスログはcommonフォーマットを使った、という解釈ですがあってますか？

あとあと、5.10.83.30のグローバルIPはElastic社公式HPのサンプルで使用しているグローバルIPを使わせて頂いてますm(_ _)m

//list[stage03_list05][Apacheのアクセスログ（サンプル）]{
5.10.83.30 - - [10/Oct/2000:13:55:36 -0700] "GET /test.html HTTP/1.0" 200 2326
//}



=== アクセスログを取り込むための準備
では、さっきの要領でLogstashを動かしてみるよ！
まずは、先ほど同様にtest02.confという名前でconfファイルを作成します。
配置先は先ほどと同様に@<code>{/etc/logstash}とします。

このtest02.confですが、inputに@<code>{file}プラグインを記載しています。
このプラグインは、インプットデータとしてファイルを指定できます。
また、ログファイルを読み込み方式指定のため、@<code>{start_position}オプションを利用してます。
デフォルト設定では@<code>{end}ですが、Logstashが起動されてから追記されたログを取り込み対象としたいので、@<code>{beginning}を定義してます。
その他にもオプションがあるので、詳しくは公式サイトのドキュメント（@<href>{https://www.elastic.co/guide/en/logstash/current/plugins-inputs-file.html}）を参照してください。


//list[stage03_list06][test02.conf]{
input {
  file {
    path => "/etc/logstash/log/httpd_access.log"
    start_position => "beginning"
  }
}
output {
  stdout { codec => rubydebug }
}
//}

次にログファイルの格納場所を作成し、ログを配置します。
#@#これは/etc配下？

//cmd{
# ログディレクトリとサンプルログを配置
$ mkdir log
$ vim log/httpd_access.log
5.10.83.30 - - [10/Oct/2000:13:55:36 -0700] "GET /test.html HTTP/1.0" 200 2326
//}

=== アクセスログを取り込むよ！
それでは作成したconfファイルを使用してログを取り込んでいきたいと思います。

ではでは、早速実行します！test01.confのときと同様に、起動スクリプトを使ってLogstashを起動します。

//cmd{
# test02.confを引数にLogstashを起動
$ /usr/share/logstash/bin/logstash -f conf.d/test02.conf
{
      "@version" => "1",
          "host" => "0.0.0.0",
          "path" => "/etc/logstash/log/httpd_access.log",
    "@timestamp" => 2017-10-01T05:33:29.689Z,
       "message" => "5.10.83.30 - - [10/Oct/2000:13:55:36 -0700] \"GET /test.html HTTP/1.0\" 200 2326"
}
//}



あれ？あれれ？？？

ログが@<code>{massage}にひとかたまりで入ってるではないですかΣ（￣Д￣;）

これはあかん。。

IPアドレス、バージョン、ステータスコードなどが別々にフィールドに入っているのが理想だったのに。。一つのフィールドに入ってしまっているOrz

理想としては、データが個別に分類されている状態をつくりたかったのです。
データ分析をするには、一つ一つのフィールドに値が入ることで集計や分析したりすることができます。
例えば、"SoruceIP"というフィールドに"5.10.83.30"という値が入るといったかたちです。
このようにデータを取得することで、IPアドレスで分析をすることが可能になります。

それでは、どのようにフィールドと値を紐付けるのでしょうか？

LogstashのFILTERを利用することで、フィールドを識別し、適切にフィールドと値を結果に反映させることができます。

ではでは、次の章はアクセスログを適切に取り込むための方法について書き書きしていきますーヽ(*ﾟдﾟ)ノ