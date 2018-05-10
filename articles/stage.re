= LogstashのGrokフィルタを極める

== Logstashのコンフィグの大まかな流れ

@<chapref>{logstash.re}のサンプルコードをみると、Logstashのコンフィグの記述はかなり複雑だということがわかります。
なので、この章ではLogstashのコンフィグをどのように記述するか、詳しく解説します。

Logstashでログを収集する流れの一例を@<img>{stage02-01}に示します。

//image[stage02-01][Logstashの構造#01]{
  Logstashの構造の図を追加
//}

@<img>{stage02-02}はLogstashの処理の流れを示したものです。LogstashはINPUTS・FILTERS・OUTPUTSの
流れでデータを処理します。

//image[stage02-02][Logstashの構造#02]{
//}


=== INPUTS
Logstashのデータソースとなるログは多様な形式で分散していることがほとんどですが、
Logstashを利用すればさまざまなデータ形式に対応できます。
たとえば、ソフトウェアのログ・サーバのメトリクス情報・Webアプリケーションのデータ・データストア・さまざまなクラウドサービスなどから
データを収集できます。


=== FILTERS
INPUTしたデータソースをLogstashのFilterで解析し、構造化します。
#@#フィールドを識別するっていきなりでてくるとわかりにくい+無くても繋がるのでとりました。入れるならfieldsについて説明が欲しいかも。
データソースの変換には、正規表現でデータをパースするためのfilterプラグイン@<code>{grok（以降Grok Filterと表記）}
やIPアドレスから地理情報を得るためのfilterプラグイン@<code>{Geoip（以降Geoip Filterと表記）}などさまざま々なフィルタライブラリ
（@<href>{https://www.elastic.co/guide/en/logstash/current/filter-plugins.html}）が用意されています。

この章ではこのGrok Filterにフォーカスして解説します。

=== OUTPUTS
データを構造化したのち、任意の出力先にデータを送付します。
Elasticsearch以外の出力先も多数提供されているので、環境に合わせてデータを送付できます。

要は、インプットデータをLogstashに食べさせると、定義したフィルタを介してデータを構造化し、出力先に指定したところに転送してくれるという感じですね。

それでは、実際にLogstashに触れていきたいと思います。

== 環境について
前章までにLogstashがすでにインストールされていることを前提とします。

== 動かす前のLogstash準備
早速Logstashを動かしてみます。

Logstashを動かすには、@<code>{logstash.conf}（以降confファイルとします）という設定ファイルを読み込ませる必要があります。
このconfファイルにINPUT・FILTER・OUTPUTを定義すると、Logstashが処理を実行します。


=== Logstashのディレクトリ構造
Logstashの設定ファイルは@<code>{/etc/logstash}に集約されています。

@<list>{stage03_list01}で
ディレクトリ構造と配置されているファイルの内容について記載しています。今回はrpmパッケージを使ってLogstashをインストールしてます。

//list[stage03_list01][/etc/logstashのディレクトリ構造]{
/etc/logstash/
├ conf.d（Logstashに実行させたいINPUT・FILTER・OUTPUTをディレクトリ配下に配置する）
├ jvm.options（ヒープサイズの割り当てなどを定義する）
├ log4j2.properties（ロギング設定）
├ logstash.yml（Logstashの設定ファイル）
└ startup.options（Logstash起動設定）
//}



=== confファイルの配置場所について
Logstashは、@<code>{logstash.yml}64行目に記載されているファイルをconfファイルとして読み込みます。
@<list>{stage03_list02}をみて分かるとおり、@<code>{*.conf}となっているため、作成するconfファイルの拡張子は@<code>{.conf}とします。
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


== Logstashを動かす

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

# 入力を受け付けている状態
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
これでLogstashの環境が整いました。


== Apacheのアクセスログを取得する

それでは早速ApacheのアクセスログをLogstashで取り込みます。

今回は@<list>{stage03_list05}のアクセスログで試してみます。
Apacheのログフォーマットは、@<code>{common}とします。


5.10.83.30のグローバルIPはElasticsearch社公式HPのサンプルで使用しているグローバルIPを利用します。

//list[stage03_list05][Apacheのアクセスログ（サンプル）]{
5.10.83.30 - - [10/Oct/2000:13:55:36 -0700] "GET /test.html HTTP/1.0" 200 2326
//}

=== アクセスログを取得するための準備

標準出力の動作時と同様、test02.confという名前でconfファイルを作成します。
配置先は@<code>{/etc/logstash}とします。

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

//cmd{
# 保存先のディレクトリを作成し、	サンプルログを配置
$ mkdir log
$ vim log/httpd_access.log
5.10.83.30 - - [10/Oct/2000:13:55:36 -0700] "GET /test.html HTTP/1.0" 200 2326
//}

=== アクセスログを取得する

test02.confを使用してログを取得します。
test01.confのときと同様に、起動スクリプトを使ってLogstashを起動します。

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



ログが@<code>{massage}にひとかたまりで挿入されています。
IPアドレス、バージョン、ステータスコードなどが別々にフィールドに入っている、つまりデータが個別に分類されている状態をつくりたかったのですが、意図した通りの動作ではなかったようです。

データ分析をするには、それぞれのフィールドに値が入ることで集計や分析ができます。
たとえば、"SoruceIP"というフィールドに"5.10.83.30"という値が入るといったかたちです。
このようにデータを取得することで、IPアドレスで分析をすることが可能になります。

それでは、どのようにフィールドと値を紐付けるのでしょうか？

LogstashはFILTERを利用することで、フィールドを識別し、適切にフィールドと値を結果に反映させることができます。

では、アクセスログを適切に取得するための方法を解説しましょう。

== Apacheのアクセスログを取得するまでのステップ

ログを適切に取得するには、FILTERでログフォーマットに合わせて定義をする必要があります。
ここではどのようにログを取得するかをステップを踏んで解説していきたいと思います。

//list[stage04_list01][ログの取り込みフロー]{
1. ログフォーマットを調べる
2. フィールド定義
3. GrokPatternをつくる
4. Grok Constructorでテスト
5. Logstashを動かしてみる
//}

手間がかかると思う方もいるかと思いますが、ひとつひとつクリアしていくことが大切です。
地味な作業が盛りだくさんですが、自分の思ったとおりにFILTERがかかったときが最高に嬉しい瞬間です！

それでは手順を個別に見ていきます。


==={01-logformat} ログフォーマットを調べる
ログは引き続き第3章のものを使用します。
Apacheのサイトにはログのフォーマットが詳細に記載されてます。

@<code>{ApacheLogFormat}（@<href>{https://httpd.apache.org/docs/2.4/en/logs.html}）

Apacheのアクセスログのログフォーマットは次のように構成されています。

#@#表形式に変更
 * LogFormat "%h %l %u %t \"%r\" %>s %b" common
 ** %h: サーバへリクエストしたクライアントIP
 ** %l: クライアントのアイデンティティ情報ですが、デフォルト取得しない設定になっているため、”-”（ハイフン）で表示される
 ** %u: HTTP認証によるリクエストしたユーザID（認証していない場合は、"-"）
 ** %t: サーバがリクエストを受け取った時刻
 ** \"%r\": メソッド、パス、プロトコルなど
 ** %>s: ステータスコード
 ** %b: クライアントに送信されたオブジェクトサイズ（送れなかった時は、"-"）


==={01-field} フィールド定義
アクセスログのログフォーマットがわかったので、フィールド名を定義していきたいと思います。
また、このときにタイプも定義しておきましょう。
()内にタイプを記載します。

#@#表形式に変更
 * %hは、クライアントIPということで"clientip"(string)
 * %lは、アイデンティティ情報なので、"ident"(string)
 * %uは、認証なので、"auth"(string)
 * %tは、時刻なので"date"(date)
 * \"%r\"は、いくつかに分割したいので、メソッドは、"verb"、パスは、"path"、んでHTTPバージョンは、"httpversion"(一式string)
 * %>sは、ステータスコードなので、"response"(long)
 * %bは、オブジェクトサイズなので、"bytes"(long)


上記がマッピングされると、@<list>{stage04_list03}のように整形されます。

//list[stage04_list02][Apacheログの整形前データ（再掲）]{
5.10.83.30 - - [10/Oct/2000:13:55:36 -0700] "GET /test.html HTTP/1.0" 200 2326
//}

#@#fluentdもゴールをここに設定（Apache）

//list[stage04_list03][Apacheログの整形後データ]{
clientip: 5.10.83.30
ident: -
auth: -
date: 10/Oct/2000:13:55:36 -0700
verb: GET
path: /test.html
httpversion: 1.0
response: 200
bytes: 2326
//}

==={04-grokpattern} GrokPatternをつくる

Grok Filterには、@<code>{GrokPattern}（@<href>{https://github.com/elastic/logstash/blob/v1.4.2/patterns/grok-patterns}）
という形であらかじめ正規表現のパターン定義が用意されているので、これを使います。

#@#Grok Patternとは、をもうすこし具体的に（参照方法・参考の仕方とか）

ただし、GrokPatternにないものは自分で作成する必要があります。これについては後述します。

それでは、ここからは各フィールドを見ながらGrokPatternを作成します。
GrokPatternを作成する、ログを左から順に攻略していくのが重要です。
これを念頭において進めます。

なお、GrokFilterの書き方は後ほど詳しく説明します。

それでは、先ほどフィールド定義した順番で解説していきます。

#@# Grok Patternで作成した正規表現をどのようにLogstashのコンフィグに埋め込むのか詳しく
#@# fluentdにもGrok Patternあるのか調べる

=== ClientIP
ClientIPということで、IPアドレスにマッチさせる必要があります。
まずは、IPアドレスにマッチさせるためのGrokPatternがすでにないか、GrokPatternのサイト上で確認します。

#@#具体例あると良い（GrokPatternのどこを参照するか？）



//list[stage04_list04][ClientIPのGrokPattern]{
IPORHOST (?:%{HOSTNAME}|%{IP})
//}


IPORHOST内は@<code>{%{HOSTNAME}}と@<code>{%{IP}}で構成されており、それぞれがGrokPatternとして定義されています。
よってHOSTNAMEとIPを別々に読み込むことが可能です。

さらにHOSTNAMEとIP自体のGrokPatternは存在するかサイトで調べてみると…ありますね！

//list[stage04_list05][HOSTNAMEのGrokPattern]{
HOSTNAME \b(?:[0-9A-Za-z][0-9A-Za-z-]{0,62})(?:\.(?:[0-9A-Za-z]
[0-9A-Za-z-]{0,62}))*(\.?|\b)
//}

//list[stage04_list05x][IPのGrokPattern]{
IP (?:%{IPV6}|%{IPV4})
//}


HOSTNAMEに正規表現が記載されていることがわかります。
また、IPは、IPv6とIPv4に対応できるように構成されてます。
これも同じ様にサイトをみると正規表現で記載されていることがわかります。

IPORHOSTでHOSTNAMEとIPが定義されていましたが、@<code>{(?:)}と@<code>{|（パイプ）}とは？と思った方もいるでしょう。
この@<code>{(?:)}は、文字列をマッチさせたい、かつキャプチャさせたくない場合に使います（キャプチャは使用しないので今回は説明を省略します）。
今回でいう文字列は、@<code>{%{HOSTNAME}}と@<code>{%{IP}に該当する文字列を指します。
また、@<code>{|}は、どちらか一方が一致した方を採用するという意味です。

結果、IPORHOSTは、HOSTNAMEかつ、IPに該当するものをマッチさせる、という設定となっています。

上記を踏まえてGrokPatternを記載すると@<list>{stage04_list06}のようになります。

//list[stage04_list06][IPORHOSTのGrokPattern]{
%{IPORHOST:clientip}
//}

@<img>{stage04-01}がイメージ図です。参考にしてみてください。

//image[stage04-01][IPアドレスをGrokするイメージ図#01]{
  Grok説明図
//}

それでは、実際にGrokがマッチするかをGrok Constructorを使って確認してみたいと思います。


=={01-grekconstructor} Grok Constructorでテスト

#@#Grok Constructorは補足にするか？（要相談）

Grok Constructor（@<href>{http://grokconstructor.appspot.com/do/match}）は、作成したGrokがマッチするかをブラウザベースでテストすることが可能なツールです。
この他にもGrokDebugger（@<href>{https://grokdebug.herokuapp.com/}）やKibanaのX-Packをインストールすることで、KibanaのDevToolsでGrokDebuggerを使ってテストもできます。

KibanaのDevToolsを使うこともできますが、手軽にGrok Filterのテストを行うためここではGrok Constructorを使用します。

Grok Constructorの使い方を@<img>{stage04-02}で解説します。

//image[stage04-02][Grok Constructorでテスト#01]{
  Grok Constructor
//}

それでは早速、先ほど作成したGrokPatternが意図どおりにマッチするかを試してみましょう。


=== clientip

//image[stage04-03][Grok Constructorでテスト#02]{
  Grok Constructor
//}

意図したとおりにclientipというフィールドに "5.10.83.30"というIPアドレスがマッチしたことがわかります。他のフィールドに対してもそれぞれ定義します。

=== ident
ユーザ名が付与されるのと@<code>{-}も含めてマッチできるものをGrokPatternで探すと@<code>{USER}というGrokPatternがあるのでこちらを使用します。

//list[stage04_list07][identのGrokPattern]{
%{USER:ident}
//}

先ほどのように、上記のGrokPatternでGrok Constructorでテストを実施するとIPアドレスが見つかります。そこで、%{IPORHOST:clientip}を含んでテストを実施してみてください。

//image[stage04-04][Grok Constructorでテスト#03]{
  Grok Constructor
//}

=== auth
authもUserと同様の定義でよいので、GrokPatternの@<code>{USER}を使用します。
また、identとauthの間もスペースがあるので@<code>{\s}もしくはスペースを入力する必要があります。
図の記載では\sを¥sで記載しています。



=== date
次は時刻です。
時刻のフォーマットは、@<code>{[day/month/year:hour:minute:second zone]}です。
これに当てはまるGrokPatternを探すと、@<list>{stage04_list08}のGrokPatternが当てはまることがわかります。

//list[stage04_list08][dateのGrokPattern]{
HTTPDATE %{MONTHDAY}/%{MONTH}/%{YEAR}:%{TIME} %{INT}
//}

こちらを使用してGrok Constructorでテストします。
先ほど作成したGrok Constructorに連ねてみましょう。

//image[stage04-05][Grok Constructorでテスト#04]{
  Grok Constructor
//}

@<code>{NOT MATCHED}と表示されています。当てはまるGrokPatternが存在しなかったようです。
実は、@<code>{%{HTTPDATE}}に該当しない@<code>{[]}があるのです。
そこで@<img>{stage04-06}の図で示しているとおり、@<code>{[]}を取り除く必要があります。
無効化するにはエスケープ（\：バックスラッシュ）を使用します。

//image[stage04-06][IPアドレスをGrokするイメージ図#02]{
  Grok
//}


=== リクエスト
次にクライアントからのリクエストです。
これは、ダブルクォーテーションの中にひとまとまりにされているので、取りたい情報を定義したフィールドにマッチできるようにGrokPatternを作成していきたいと思います。

//list[stage04_list09][リクエストのGrokPattern]{
"GET /test.html HTTP/1.0"
//}

まず、GETですが、GETという文字列以外にもPOSTや、DELETEなどがあります。
単純にGETという固定文字でマッチングすることはできません。
また、GET|PUT|DELETEなども、よくありません。
汎用的にすることと、可読性を意識してマッチングさせたいためです。

ということで、英単語が入るということがわかっているので、\bxxx\b（xxxは何かの英単語）に該当するGrokPatternを使用します。

これまでどおり、GrokPatternを探すと次のように該当しますね。

//list[stage04_list10][英単語のGrokPattern]{
WORD \b\w+\b
//}

次にパスですが、リクエストによって変動したりするため、柔軟性を求めて@<list>{stage04_list11}のようにNOTSPACEを使用します。
NOTSPACEは、空白文字以外にマッチのため、空白文字が出現するまでマッチします。

//list[stage04_list11][NOTSPACEのGrokPattern#01]{
NOTSPACE \S+
//}

最後のHTTPのバージョンですが、HTTP部分は不要なので取り除くのと、そもそも、HTTPバージョンがはいっていないパターンもあります。
そんな時は、@<code>{(?:)?}を利用することで、このGrokPatternにマッチする時は使うけれど、マッチしない時は使わない、といった定義ができるのです！
これは、便利なので覚えて置いてください。
最後に最短マッチとして、@<code>{%{DATA\}}もパイプで組み込んでます。

#@#Re:VIEWの@<>{}で}が必要なときはバックスラッシュ

//list[stage04_list12][NOTSPACEのGrokPattern#02]{
(?: HTTP/%{NUMBER:httpversion})?|%{DATA:rawrequest})"
//}

ここまでを@<img>{stage04-07}にまとめました。

//image[stage04-07][IPアドレスをGrokするイメージ図#03]{
  Grokパワポ
//}


=== response & bytes
ここまできたらあと少しです。

responseはステータスコードなので、@<code>{NUMBER}を使用します。
また、bytesも同様に@<code>{NUMBER}を使用しますが、オブジェクトが送れなかった場合は@<code>{-}になるため、@<code>{|}で@<code>{-}を追加します。

これで全ての準備が整ったので、Grok Constructorでテストしたいと思います。


=== Grok Constructor全体テスト
@<list>{stage04_list13}のGrokPatternでテストをします。実際は改行しませんが、本文の都合上適宜改行しています。

//list[stage04_list13][最終的なGrokPattern]{
%{IPORHOST:clientip} %{USER:ident} %{USER:auth} \[%{HTTPDATE:date}\]
"(?:%{WORD:verb} %{NOTSPACE:path}(?: HTTP/%{NUMBER:httpversion})?|%{DATA:rawrequest})"
%{NUMBER:response} (?:%{NUMBER:bytes}|-)
//}

//image[stage04-08][Grok Constructorでテスト#04]{
  Grok Constructor
//}

問題なくマッチしましたね！


=={01-logstash} logstashを動かしてみる
やっとここでLogstashのconfファイルが登場します。
それでは、confファイルを作成します。

今まではINPUTとOUTPUTのみでしたが、先ほど作成したGrokPatternを埋め込みたいので、FILTERを追加します。
GrokPatternをFILTERに直接コーディングすることも可能ですが、可読性を意識したいため、GrokPatternをconfファイルとして外出しします。

外出しするために、次の作業を実施します。

#@#ファイル名をvimで編集するファイルに追記（%{IPORHOST}の前）

//cmd{
# GrokPatternファイルを配置するためのディレクトリを作成
$ mkdir patterns
# httpd用のGrokPatternファイルを作成
# GrokPattern名をHTTPD_COMMON_LOGとします
$ vim patterns/httpd_patterns
%{IPORHOST:clientip} %{USER:ident} %{USER:auth} \[%{HTTPDATE:date}\]
"(?:%{WORD:verb} %{NOTSPACE:path}(?: HTTP/%{NUMBER:httpversion})?|%{DATA:rawrequest})"
%{NUMBER:response} (?:%{NUMBER:bytes}|-)
//}

次に、GrokPatternファイルを作成したので、ログの変換をさせるためとGrokPatternを読み込むためにLogstashのconfファイルに以下を記載します。
ファイル名はtest03.confとしました。

//list[stage04_list14][test03.conf]{
input {
  file {
    path => "/etc/logstash/log/httpd_access.log"
    start_position => "beginning"
  }
}
filter {
  grok {
    patterns_dir => ["/etc/logstash/patterns/httpd_patterns"]
    match => { "message" => "%{HTTPD_COMMON_LOG}" }
  }
output {
  stdout { codec => rubydebug }
}
//}

#@#これはべんりだね

それでは、実行してみます。

//cmd{
$ usr/share/logstash/bin/logstash -f conf.d/test03.conf
# 出力結果！！
{
        "request" => "/test.html",
           "auth" => "-",
          "ident" => "-",
           "verb" => "GET",
        "message" => "5.10.83.30 - - [10/Oct/2000:13:55:36 -0700] \"GET /test.html HTTP/1.0\" 200 2326",
           "path" => "/etc/logstash/log/httpd_access.log",
     "@timestamp" => 2017-10-01T15:11:19.695Z,
       "response" => "200",
          "bytes" => "2326",
       "clientip" => "5.10.83.30",
       "@version" => "1",
           "host" => "0.0.0.0",
    "httpversion" => "1.0",
      "timestamp" => "10/Oct/2000:13:55:36 -0700"
}
//}

意図どおりにフィールドが抽出できました。
しかし、ログのタイムスタンプではなく、ログを取り込んだ時刻になっているので、修正が必要です。
また、グローバルIPがあるのならば、地域情報とマッピングしたいところです。
ということで、Logstashのconfファイルを修正したいと思います。

//list[stage04_list15][test03.conf（修正後）]{
input {
  file {
    path => "/etc/logstash/log/httpd_access.log"
    start_position => "beginning"
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
  stdout { codec => rubydebug }
//}

各々のフィルターについて@<img>{stage04-09}と合わせて説明します。

//image[stage04-09][Logstash.confの説明]{
  Logstash.conf
//}

 1. ファイルの読み込み位置を指定し、Logstash起動前のログも対象としたいため、"biginning"としています
 2. パターンファイルの読み込み
 3. messageフィールドに格納されている値を”HTTPD_COMMON_LOG”でマッチングします
 4. パターンファイル内でIPアドレスをマッチングさせているclientipフィールドを対象にgeoipフィルタを利用し、地理情報を取得します
 5. Logstashは、ログデータを取り込んだ時間を@timestampに付与するので、dateフィルタを用いることで実際のログデータのタイムスタンプを付与することができます
 6. パターンファイル内のdateフィールドに対して定義したdateパターンとマッチする場合に値を書き換えます
 7. 日付の月が"Oct"になっているため、localeを"en"に指定しています
 8. 変更を変えたいターゲットとして"@timestamp"を指定します
 9. 不要なフィールドをremove_fieldで指定し、削除します（容量を抑えるためと不必要な情報を与えないため）

それでは、修正したconfファイルで再度実行すると次のようになります。
地理情報やタイムスタンプや不要な情報が削除されていることがわかります。

//cmd{
{
        "request" => "/test.html",
          "geoip" => {
              "timezone" => "Europe/Amsterdam",
                    "ip" => "5.10.83.30",
              "latitude" => 52.35,
        "continent_code" => "EU",
             "city_name" => "Amsterdam",
          "country_name" => "Netherlands",
         "country_code2" => "NL",
         "country_code3" => "NL",
           "region_name" => "North Holland",
              "location" => {
            "lon" => 4.9167,
            "lat" => 52.35
        },
           "postal_code" => "1091",
           "region_code" => "NH",
             "longitude" => 4.9167
    },
           "auth" => "-",
          "ident" => "-",
           "verb" => "GET",
     "@timestamp" => ”210/Oct/2000:13:55:36 -0700”
       "response" => "200",
          "bytes" => "2326",
       "clientip" => "5.10.83.30",
       "@version" => "1",
    "httpversion" => "1.0",
}
//}


これでGrokを利用してApacheアクセスログを抽出できるようになりましたね！
ビジュアライズしたい場合などは、OUTPUTをElasticsearchにし、Kibanaでインデックスを参照することでビジュアライズが可能です。

次は、すでに存在しているGrokPatternだけでは取り込めないログをベースに説明していきたいと思います。

なお、Grok Constructorで、作成したGrokPatternをテストすることも可能です。
@<img>{stage04-10}にあるとおりにテストして頂ければと思います。

//image[stage04-10][Grok Constructorでテスト#05]{
  Grok Constructor
//}

== 今度は何を取得する？

前項でApacheのアクセスログを取り込めるようになりました。
次に、既存のGrokPatternだけではどうにもならないログを対象にGrokしていきたいと思います。

GrokPattenはJava、bind、Redisなどさまざまなものが用意されてます。
また、FireWallという括りでCiscoのASAのGrokPatternが用意されているものもあります。
ただ、すべてがまかなえてるかというと、そうではありません。

今回はCiscoのファイアウォール製品であるASAのログを取り込みます。
やっぱり企業を守っているファイアウォールがどんなログを出しているか気になりますよね？
使えるGrokPatternは積極的に使いましょう。


といことで今回は@<list>{stage05-list01}のログを対象にしたいと思います。
IPアドレスは、適当なプライベートIPアドレスを割り当てています。

//list[stage05-list01][Cisco ASAのログ]{
Jun 20 10:21:34 ASA-01 : %ASA-6-606001: ASDM session number 0
 from 192.168.1.254 started
Jun 20 11:21:34 ASA-01 : %ASA-6-606002: ASDM session number 0
 from 192.168.1.254 ended
//}



いつもどおりに@<list>{stage05_list02}のログ取り込みフローで進めたいと思います！


//list[stage05_list02][ログの取り込みフロー（再掲）]{
1. ログフォーマットを調べる
2. フィールド定義
3. GrokPatternをつくる
4. Grok Constructorでテスト
5. Logstashを動かしてみる
//}


==={02-logformat} ログフォーマットを調べる
まずは、ログフォーマットを調べます。
Ciscoさんは丁寧にログフォーマットを掲載してます（URL:@<href>{https://www.cisco.com/c/en/us/td/docs/security/asa/syslog/b_syslog.html}）。

……よく見るとわかりますが、数が多いです。Ciscoは世界最大のメーカーですからね。

まず該当するログフォーマットを探す方法ですが、@<list>{stage05-list03}のログに"%ASA-6-606001"という記載がありますので、このイベントNo.の"606001"で検索することができます。

このログフォーマットは@<list>{stage05-list03}のようになっています。

//list[stage05-list03][ASAのログフォーマット]{
%ASA-6-606001: ASDM session number number from IP_address started
%ASA-6-606002: ASDM session number number from IP_address ended
//}



ASDM（Webベースの管理インターフェースを提供するツール）のセッションを開始した時と終了した時に出力するログですね。


==={02-field} フィールド定義
では、フィールド定義です。ログの左から順にみていきます。
先ほど見たログフォーマットには、タイムスタンプとASA-01という項目がありませんでした。
これらは、ログに必ず記載されるので、こちらも定義します。

#@#表形式に変更

 * Jun 20 10:21:34 ASA-01 : %ASA-6-606001: ASDM session number 0 from 192.168.1.254 started
 ** timestamp: Jun 20 10:21:34 (date)
 ** hostname: ASA-01 (string)
 ** eventid: ASA-6-606001 (string)
 ** ASDM-sesion-number: 0 (long)
 ** src_ip: 192.168.1.254 (string)
 ** status: started (string)

実際のログに記載されているメッセージ内容のすべてが、フィールドにマッピングされていないことがわかります。
たとえば、@<code>{ASDM session number}というメッセージに対して意味はなく、そのセッションナンバーが知りたいのです。
そのため、フィールド名に@<code>{ASDM session number}とし、値としては取り込まないようにします。
その他の@<code>{from}も同様で、どこからのIPアドレスかを知りたいため、fromを取り除き、@<code>{src_ip}（ソースIP）というフィールドにIPアドレスを値として取り込みます。

2つ目のログ（@<list>{stage05-list03}の2行目）ですが、最後の@<code>{ended}しか変わらないということがわかります。先ほどのフィールド定義をそのまま使用するので割愛します。


==={05-grokpattern} GrokPatternをつくる
それでは、GrokPatternを作っていきます。


=== 共通部分
タイプスタンプとホスト名、イベントIDはすべてのログに入るメッセージのため、共通部分とします。
それでは、タイムスタンプとホスト名、イベントIDに取り掛かります。

タイムスタンプは、GrokPatternに @<code>{CISCOTIMESTAMP}を使用します。

//list[stage05_list04][CISCOTIMESTAMPのGrokPattern]{
CISCOTIMESTAMP %{MONTH} +%{MONTHDAY}(?: %{YEAR})? %{TIME}
//}


また、ホスト名は、ユーザが自由に付与する名前のため、柔軟性を求めて@<code>{NOTSPACE}を使用します。
また、先頭にスペースが必ず入るので@<code>{\s}を入れます。


//list[stage05_list05][HOSTNAMEのGrokPattern]{
HOSTNAME \s%{NOTSPACE:hostname}
//}


イベントIDは、GrokPatternに用意されていないので、自分で作成します。
自分でGrokPatternを作成する場合は次のように作成します。

#@#表形式に変更
 * (?<hostname>ASA-\d{1}-\d{6})
 ** GrokPatternを作成したい場合は、(?)で括り、<>内にフィールド名を任意に付与します
 ** それ以降（ここでいうASAから始まる正規表現）にフィールドに入れたい正規表現を記載します


上記のように作成することで好きなGrokPatternを作成することができます。
これをCustomPatternといいます。


== 固有部分
ここからはイベント毎に異なる固有部分のGrokPatternを作っていきます。
共通部分を取り除いた部分の以下が対象ですね。


//list[stage05_list08][イベントごとに異なる部分のログ（抜粋）]{
: ASDM session number 0 from 192.168.1.254 ended
//}



=== ASDMセッションNo.
フィールド定義で記載したとおりですが、ASDMセッションNo.をフィールドとして値が取得できればよいので、@<list>{stage05_list09}のようになります。

//list[stage05_list09][ASDMセッションNoのGrokPattern]{
ASDM session number(?<ASDM-sesion-number>\s[0-9]+)
//}


これも見て分かるとおり、CustomPatternで作成しています。
それぞれについてみていくと@<code>{(?)}の外に@<code>{ASDM session nubber}がありますね。
これは、@<code>{ASDM session nubber}をマッチしても値は取得したくない場合に使う方法です。
そこで、隣の@<code>{(?<ASDM-session-number>\s[0-9]+)}というCustomPatternで取得した値が
@<code>{ASDM-session-number}というフィールドに入ります。
正規表現部分は、@<code>{\s}のスペースと@<code>{0-9}の数字が複数並んでも対応できるように@<code>{+}を使用してます。

最終的に先頭の@<code>{:}とスペースも含むので以下な感じになります。

//list[stage05_list10][ASDMセッションNoのGrokPattern（完成版）]{
:\sASDM session number(?<ASDM-session-number>\s[0-9]+)
//}



=== ソースIPアドレス
これはApacheのアクセスログと同じですね。
IPアドレスのGrokPatternのように他にも確立されているものは積極的に使っていきましょう。

* from 192.168.1.254

これは、フィールド定義で説明したようにソースIPなので、GrokPatternの@<code>{IP}を使用し、不要な部分を取り除く必要があります。
スペースと@<code>{from}が不要なのでGrokPatternの外側に出しますが、ひとつの文字列とするため()で囲います。

//list[stage05_list11][ソースIPアドレスのGrokPattern]{
(\sfrom\s%{IP:src_ip})
//}

=== ステータス
最後は、セッションステータスを表す@<code>{started}ですね。
これは、CustomPatternで対応します。
先ほどのソースIPとの間にスペースがあるので@<code>{\s}を入れます。
また、@<code>{started}は文字列なので@<code>{\b}を入れて以下な感じです。

//list[stage05_list12][セッションステータスのGrokPattern]{
\s(?<session>\bstarted)
//}


ただ、もうひとつのイベントID"606002"ですが、ステータスがendedしか変わりません。
そこで、先ほどのステータスに"started""ended"のどちらかを選択できるようにします。

//list[stage05_list13][ステータスの選択を可能にする]{
\s(?<session>\bstarted|\bended)
//}


@<code>{|}を入れることで選択できるようになります。
これで整ったので、GrokConstructorでテストをしてみたいと思います。


=={02-grekconstructor} Grok Constructorでテスト
パターンファイルを抽出し、テストを実施します。

//list[stage05_list14][パターンファイルまとめ]{
CISCOTIMESTAMP %{MONTH} +%{MONTHDAY}(?: %{YEAR})? %{TIME}
EVENTID \s: %(?<EventID>ASA-\d{1}-\d{6})
CISCOFW606001 :\sASDM\ssession\snumber(?<ASDM-session-number>\s[0-9]+)
(\sfrom\s%{IP:src_ip})\s(?<session>\bstarted|\bended)
//}


//list[stage05_list15][Grock用の設定]{
%{CISCOTIMESTAMP:date}\s%{NOTSPACE:hostname}%{EVENTID}%{CISCOFW606001}
//}


実行結果が@<img>{stage05-01}です。

//image[stage05-01][ASA Grok Constructor結果#01]{
  Grok Constructor
//}


=={02-logstash} logstashを動かしてみる

ここまできたらあと少し！ということでApacheのアクセスログのときと同様にconfファイルを作成します。

今回もパターンファイルに外出しします。

=== パターンファイル
タイムスタンプやホスト名、イベントIDそしてイベントメッセージのGrokPatternをパターンファイルに定義します。
GrokPatternの"CISCOFW606001"に"606002"も含んでいるのですが、文字数を短くするために"606001"に集約してます。
ただし、含んでいることがわかりにくいと思う方は変更しても問題ありません。

//cmd{
$ vim patterns/asa_patterns
CISCOTIMESTAMP %{MONTH} +%{MONTHDAY}(?: %{YEAR})? %{TIME}
EVENTID \s: %(?<EventID>ASA-\d{1}-\d{6})
CISCOFW606001 :\sASDM\ssession\snumber(?<ASDM-session-number>\s[0-9]+)
(\sfrom\s%{IP:src_ip})\s(?<session>\bstarted|\bended)
//}

これでパターンファイルの準備は完了です。

補足ですが、パターンファイルをGrok Constructorでテストすることも可能です。
@<img>{stage05-02}は実際に作成したパターンファイルでテストを実施した結果です。

//image[stage05-02][ASA Grok Constructor結果#02]{
  Grok Constructor
//}


==={06-logstash} logstash.conf

Apacheの際と同様に作成したものが@<list>{stage05_list16}です。ファイル名はasa.confとして保存しました。

//list[stage05_list16][asa.conf]{
input {
  file {
  	path => "/etc/logstash/log/asa.log"
  	start_position => "beginning"
  }
}
filter {
  grok {
  	patterns_dir => ["/etc/logstash/patterns/asa_patterns"]
  	match => { "message" => "%{CISCOTIMESTAMP:date}\s
    %{NOTSPACE:hostname}%{EVENTID}%{CISCOFW606001}" }
  }
  date {
    match => ["date", "MMM dd HH:mm:ss", "MMM  d HH:mm:ss" ]
  }
  mutate {
    remove_field => ["date", "message", "path", "host"]
  }
}
output {
  stdout { codec => rubydebug }
}
//}

実行結果は次のようになります。

//cmd{
# EventID: 606001
{
                 "src_ip" => "192.168.1.254",
               "hostname" => "ASA-01",
             "@timestamp" => 2017-06-20T02:21:34.000Z,
                "session" => "started",
               "@version" => "1",
    "ASDM-session-number" => " 0",
                "EventID" => "ASA-6-606001"
}

# EventID: 606002
{
                 "src_ip" => "192.168.1.254",
               "hostname" => "ASA-01",
             "@timestamp" => 2017-06-20T02:21:34.000Z,
                "session" => "ended",
               "@version" => "1",
    "ASDM-session-number" => " 0",
                "EventID" => "ASA-6-606001"
}
//}

想定どおりにログを抽出できましたね！

GrokPatternにあるものは積極的に使用し、ないものはCustomPatternで作る！といったことを学習できたのではないでしょうか。
今回はCiscoのASAを取り上げましたが、この他のログも同様に対応していくことが可能です。
さまざまなログにトライしてみてください！

== AWSのログを取得する
AWSサービスはログを出力する機能をもったサービスがあります。そのなかでも今回はELBアクセスログをLogstashで取り込みます。

@<img>{stage06-01}はAWSサービスのログを取得するイメージです。

//image[stage06-01][ELBログ取得イメージ]{
  ELB
//}

この他にもCloudtrailやS3などもログを出力し、取得することが可能です。


#@#ALBに変更

== ELBのログを取得するよ！
これまでに説明した"ログ取り込みフロー"に沿って進めたいと思います。


== ログフォーマットを調べる
ELBのログフォーマットを調べます。

前提としてELBのロギングは有効化されていることとします。
もし設定されていない方は、公式ドキュメントを確認ください。

公式ドキュメントにClassic Load Balancerのアクセスログのフォーマット
（@<href>{http://docs.aws.amazon.com/ja_jp/elasticloadbalancing/latest/classic/access-log-collection.html}）が記載されています。


公式ドキュメントの記載をひとつひとつ分解していきます。

#@#表形式に変更
 * timestamp elb client:port backend:port request_processing_time backend_processing_time response_processing_time elb_status_code backend_status_code received_bytes sent_bytes "request" "user_agent" ssl_cipher ssl_protocol
 ** timestamp: ロードバランサーがクライアントからリクエストを受け取った時刻 (ISO 8601 形式)
 ** elb: ロードバランサーの名前
 ** client:port: リクエストを送信したクライアントの IP アドレスとポート
 ** backend:port: ELBにぶら下がっているインスタンス（バックエンド）に対してのIPアドレスとポート番号（リクエストが送信できなかった場合は"-"）
 ** request_processing_time: ELBがリクエストを受け取ってから、バックエンドのインスタンスに送信するまでの時間(応答がない場合などは"-1")
 ** backend_processing_time: ELBがバックエンドにリクエストを送信してから、レスポンスが帰ってくるまでの時間(応答がない場合などは"-1")
 ** response_processing_time: ELBがレスポンスを受け取ってから、クライアントに返すまでの時間(応答がない場合などは"-1")
 ** elb_status_code: ELBからのレスポンスステータスコード
 ** backend_status_code: バックエンドのインスタンスからのレスポンスステータスコード
 ** received_bytes: クライアントから受信したリクエストサイズ
 ** sent_bytes: クライアントに送信したリクエストサイズ
 ** request: クライアントからのリクエスト（HTTP メソッド + プロトコル://ホストヘッダー:ポート + パス + HTTP バージョン）
 ** user_agent: リクエスト元のクライアントを特定する
 ** ssl_cipher: SSL暗号化(暗号化されていない場合は"-")
 ** ssl_protocol: SSLプロトコル(暗号化されていない場合は"-")

#@#表形式に変更

== フィールド定義
まずフィールド定義です。

今回は、Apacheのアクセスログと違ってすでにフィールド名が公式として定義されているので、そのまま使用します。
ただし、client:portのようなフィールドは、clientipとportに分割します。その他のbackendやrequestも分割します。

それではフィールドのタイプを決めるために、サンプルログから当てはめます
サンプルログは、先ほどのリンクのAWS公式ドキュメントから使っています。

#@#表形式に変更
 * 2015-05-13T23:39:43.945958Z my-loadbalancer 5.10.83.30:2817 10.0.0.1:80 0.000073 0.001048 0.000057 200 200 0 29 "GET http://www.example.com:80/ HTTP/1.1" "curl/7.38.0" - -
 ** timestamp: 2015-05-13T23:39:43.945958Z (date)
 ** elb: my-loadbalancer (string)
 ** client_ip: 5.10.83.30 (string)
 ** client_port: 2817 (string)
 ** backend_ip: 10.0.0.1 (string)
 ** backend_port: 2817 (string)
 ** request_processing_time: 0.000073 (float)
 ** backend_processing_time: 0.001048 (float)
 ** response_processing_time: 0.000057 (float)
 ** elb_status_code: 200 (string)
 ** backend_status_code: 200 (string)
 ** received_bytes: 200 (long)
 ** sent_bytes: 0 (long)
 ** verb: GET (string)
 ** proto: http (string)
 ** urihost: www.example.com:80 (string)
 ** uripath: - (string)
 ** httpversion: 1.1 (string)
 ** user_agent: curl/7.38.0 (string)
 ** ssl_cipher: - (string)
 ** ssl_protocol: - (string)


このようにマッピングされるようにGrokPatternを作成します。


=={06-grokpattern} GrokPatternをつくる
前項のようにGrokPatternを作っていきましょう。
実は、AWSのELBにはGrokPatternが用意されているのです。

ただ、それを使うだけではなく、理解して自由に使えるようになりましょう。


=== timestamp
ELBの時刻形式は、ISO8601のフォーマットを利用しています。
そのため、GrokPatternに存在する@<code>{TIMESTAMP_ISO8601}をそのまま使用できるため、こちらを使います。

//list[stage06_list01][timestampのGrokPattern]{
%{TIMESTAMP_ISO8601:date}
//}

=== elb
elbの名前です。
ユーザが任意につける名前なので、GrokPatternの @<code>{NOTSPACE}を使用します。

//list[stage06_list02][elbのGrokPattern]{
%{NOTSPACE:elb}
//}



=== client_ip & client_port
Apacheアクセスログと同様に@<code>{IPORHOST}を使用したくなりますが、ここでは行いません。
なぜなら@<code>{IPORHOST}はIPだけでなくHOSTも含んでいるためです。
今回のフィールドはIPのみのため、@<code>{client_ip}はGrokPatternのIPとし、@<code>{client_port}は@<code>{INT}とします。

//list[stage06_list03][client_ipとclient_portのGrokPattern]{
(?:%{IP:client_ip}:%{INT:client_port:int})
//}



=== backend_ip & backend_port
上記のclient_ipとclinet_port同様です。
しかし、バックエンドから応答がない場合は、@<code>{-}となるため、@<code>{|}で@<code>{-}も記載します。

//list[stage06_list04][backend_ipとbackend_portのGrokPattern]{
(?:%{IP:backend_ip}:%{INT:backend_port:int}|-)
//}


=== リクエストタイム3兄弟
これらすべてGrokPatternの @<code>{NUMBER}を使用し、応答がなかった場合に@<code>{|}で @<code>{-1}も記載します。
このフィールドを利用することで、ELBが受け付けてからのレイテンシを測ることができます。

//list[stage06_list05][リクエストタイム用のGrokPattern]{
(?:%{NUMBER:request_processing_time:double}|-1)
(?:%{NUMBER:backend_processing_time:double}|-1)
(?:%{NUMBER:response_processing_time:double}|-1)
//}



=== elb_status_code & backend_status_code
Apacheのアクセスログと同様にステータスコードは、 @<code>{NUMBER}を使用します。

//list[stage06_list06][elb_status_codeとbackend_status_code用のGrokPattern]{
(?:%{INT:elb_status_code}|-)
(?:%{INT:backend_status_code:int}|-)

//}


=== received_bytes & sent_bytes
バイトも同様に @<code>{NUMBER}を使用します。


//list[stage06_list07][received_bytesとsent_bytes用のGrokPattern]{
%{INT:received_bytes:int}
%{INT:sent_bytes:int}
//}


=== request
requestの中に複数のフィールドが組み込まれています。
GrokPatternをみると@<code>{ELB_REQUEST_LINE}という設定があります。
このGrokPatternは、"verb" "proto" "urihost" "uripath" "httpversion"を含んでいます。
#@#これはログの文字列なのか、設定の名前なのかわからなかったのでそのまま。設定なら@<code>{}
そのため、@<code>{ELB_REQUEST_LINE}を呼び出すだけでマッチさせることができます。
察しのいい方は気づいているかもしれませんが、GrokPatternの中で更にGrokPatternを呼び出すことができます。


//list[stage06_list08][request用のGrokPattern]{
ELB_REQUEST_LINE (?:%{WORD:verb} %{ELB_URI:request}
(?: HTTP/%{NUMBER:httpversion})?|%{DATA:rawrequest})
//}


上記の@<code>{ELB_REQUEST_LINE}内で@<code>{ELB_URI}を呼び出しています。

//list[stage06_list09][ELB_URI用のGrokPattern]{
ELB_URI %{URIPROTO:proto}://(?:%{USER}(?::[^@]*)?@)?
(?:%{URIHOST:urihost})?(?:%{ELB_URIPATHPARAM})?
//}


更に、@<code>{ELB_URIPATHPARAM}というのを呼び出しているかたちになっています。

//list[stage06_list10][ELB_URIPATHPARAM用のGrokPattern]{
ELB_URIPATHPARAM %{URIPATH:path}(?:%{URIPARAM:params})?
//}


=== user_agent
Apacheアクセスログで使用したGrokPatternの@<code>{DATA}を使用します。
@<code>{GREEDYDATA}というGrokPatternもあるのですが、最長マッチになってしまうため、想定外のものとマッチしてしまうため、@<code>{DATA}を使用します。


//list[stage06_list11][user_agent用のGrokPattern]{
(?:%{DATA:user_agent}|-)
//}


=== ssl_cipher & ssl_protocol
SSL通信時に使用するフィールドで、使用していない場合は、@<code>{-}が付くため@<code>{|}を記載します。


//list[stage06_list12][ssl_cipherとssl_protocol用のGrokPattern]{
(?:%{NOTSPACE:ssl_cipher}|-)
(?:%{NOTSPACE:ssl_protocol}|-)
//}


== Grok Constructorでテスト
個々のテスト結果は省いてますが、慣れるまでは一つ一つクリアしていってください！
ちなみに、今回作成したGrokPattern名がELBではなくCLBなのは、Application Load Balancer（以下、ALB）と区別するためです。
ALBとCLBでは、ログフォーマットが若干違うため、区別しています。

ALB版も合わせてGrokPatternを記載します。


//list[stage06_list13][キャプション]{
CLB_ACCESS_LOG %{TIMESTAMP_ISO8601:date}\s%{NOTSPACE:elb}
\s(?:%{IP:client_ip}:%{INT:client_port:int})
\s(?:%{IP:backend_ip}:%{INT:backend_port:int}|-)
\s(?:%{NUMBER:request_processing_time}|-1)
\s(?:%{NUMBER:backend_processing_time}|-1)
\s(?:%{NUMBER:response_processing_time}|-1)
\s(?:%{INT:elb_status_code}|-)\s(?:%{INT:backend_status_code:int}|-)
\s%{INT:received_bytes:int} %{INT:sent_bytes:int}\s\"%{ELB_REQUEST_LINE}\"
\s\"(?:%{DATA:user_agent}|-)\"\s(?:%{NOTSPACE:ssl_cipher}|-)
\s(?:%{NOTSPACE:ssl_protocol}|-)
ALB_ACCESS_LOG %{NOTSPACE:type}\s%{TIMESTAMP_ISO8601:date}
\s%{NOTSPACE:elb}\s(?:%{IP:client_ip}:%{INT:client_port})
\s(?:%{IP:backend_ip}:%{INT:backend_port}|-)
\s(:?%{NUMBER:request_processing_time}|-1)
\s(?:%{NUMBER:backend_processing_time}|-1)
\s(?:%{NUMBER:response_processing_time}|-1)
\s(?:%{INT:elb_status_code}|-)\s(?:%{INT:backend_status_code}|-)
\s%{INT:received_bytes} %{INT:sent_bytes}\s\"%{ELB_REQUEST_LINE}\"
\s\"(?:%{DATA:user_agent}|-)\"\s(?:%{NOTSPACE:ssl_cipher}|-)
\s(?:%{NOTSPACE:ssl_protocol}|-)\s%{NOTSPACE:target_group_arn}
\s\"%{NOTSPACE:trace_id}\"
//}

#@#長すぎてきれちゃうのでキリのいいところで改行入れて欲しい

CLBのGrok Constructorの結果です。

//image[stage06-02][CLB Grok Constructor結果]{
  Grok Constructor
//}


== logstashを動かしてみる
ここからconfファイルの作成ですが、Apacheのアクセスログと構造はほぼ同じです。
ただ、大きく違うのがINPUTがファイルパスではなく、S3からという点です。

それでは、S3をINPUTにした取り込み方法について解説していきたいと思います。
FILTERとOUTPUTに関しては、最終的なconfファイルを記載するかたちとします。

=== Install S3 Plugin
S3をINPUTとしてログを取得するには、追加でプラグインをインストールする必要があります。
#@#Logstashに追加プラグインをインストールする方法を追記

//list[stage06_list14][logstash-input-s3のインストール]{
/usr/share/logstash/bin/logstash-plugin install logstash-input-s3
//}

実行すると、次のように出力されます。

//cmd{
/usr/share/logstash/bin/logstash-plugin install logstash-input-s3
Validating logstash-input-s3
Installing logstash-input-s3
Installation successful
//}

また、S3にアクセスできるようにIAM Roleの設定がされていることを前提としています。

=== logstash.conf
準備が整ったので以下にlogstash.confを記載します。


//list[stage06_list15][作成したlogstash.conf]{

input {
  file {
    path => "/Users/micci/project/logstash-5.5.2/clb.log"
    start_position => "beginning"
  }
}
filter {
  grok {
    patterns_dir => ["/Users/micci/project/logstash-5.5.2/patterns/clb_patterns"]
    match => { "message" => "%{CLB_ACCESS_LOG}" }
  }
  date {
    match => [ "date", "ISO8601" ]
    timezone => "Asia/Tokyo"
    target => "@timestamp"
  }
  geoip {
    source => "client_ip"
  }
  mutate {
    remove_field => [ "date", "message", "path"m ]
  }
}
output {
  stdout { codec => rubydebug }
}
//}

実行すると、次のように出力されます。

//cmd{
{
              "received_bytes" => 0,
                     "request" => "http://www.example.com:80/",
                  "backend_ip" => "10.0.0.1",
                  "ssl_cipher" => "-",
                "backend_port" => 80,
                  "sent_bytes" => 29,
                 "client_port" => 2817,
         "backend_status_code" => 200,
                    "@version" => "1",
                        "host" => "122x208x2x42.ap122.ftth.ucom.ne.jp",
                         "elb" => "my-loadbalancer",
                   "client_ip" => "5.10.83.30",
     "backend_processing_time" => "0.001048",
                  "user_agent" => "curl/7.38.0",
                "ssl_protocol" => "-",
                       "geoip" => {
              "timezone" => "Europe/Amsterdam",
                    "ip" => "5.10.83.30",
              "latitude" => 52.35,
        "continent_code" => "EU",
             "city_name" => "Amsterdam",
          "country_name" => "Netherlands",
         "country_code2" => "NL",
         "country_code3" => "NL",
           "region_name" => "North Holland",
              "location" => {
            "lon" => 4.9167,
            "lat" => 52.35
        },
           "postal_code" => "1091",
           "region_code" => "NH",
             "longitude" => 4.9167
    },
             "elb_status_code" => "200",
                        "verb" => "GET",
     "request_processing_time" => "0.000073",
                     "urihost" => "www.example.com:80",
    "response_processing_time" => "0.000057",
                  "@timestamp" => 2015-05-13T23:39:43.945Z,
                        "port" => "80",
                       "proto" => "http",
                 "httpversion" => "1.1"
}
//}

AWSのサービスに対してもログを取り込めるということがわかったのではないでしょうか。
この他のサービスに対してもトライして頂ければと思います。

本章を通じてGrokに対して少しでも苦手意識がなくなりましたか？
少しでも苦手意識がなくなれば筆者としては嬉しい限りです。

とはいえ、ログを取得する作業というのは試行錯誤の連続です。
ログのパース失敗があれば改修する必要がありますし、ログフォーマットが変われば改修する必要があります。

世界中のログをみんなで力を合わせてパターンを増やしていければと思っています。
