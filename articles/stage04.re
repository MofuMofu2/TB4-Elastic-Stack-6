= Apacheのアクセスログを取り込もう！
#@#理由以下略（まじすまん）
== ログを取り込むまでのステップ
ログをよしなに取り込むにはFILTERでログフォーマットに合わせて定義をする必要があります。
なので、この章ではどのようにログを取り込むかをステップを踏んで解説していきたいと思います。

//list[stage04_list01][ログの取り込みフロー]{
1. ログフォーマットを調べる
2. フィールド定義
3. GrokPatternをつくる
4. Grok Constructorでテスト
5. Logstashを動かしてみる
//}

結構、ステップ面倒いなーって思う人もいるかと思いますが、一つ一つクリアしていくことが大切だと思ってます。
地味な作業が盛りだくさんですが、自分の思った通りにFILTERがかかったときが最高に嬉しい瞬間です！

それでは個別に見ていきたいとおもいまするー


=== ログフォーマットを調べる
ログは引き続き第3章のものを使用します。
Apacheのサイトにはログのフォーマットが詳細に記載されてます。

@<code>{ApacheLogFormat}（@<href>{https://httpd.apache.org/docs/2.4/en/logs.html}）

Apacheのアクセスログのログフォーマットは次のような感じで構成されています。

#@#表形式に変更
 * LogFormat "%h %l %u %t \"%r\" %>s %b" common
 ** %h: サーバーへリクエストしたクライアントIP
 ** %l: クライアントのアイデンティティ情報ですが、デフォルト取得しない設定になっているため、”-”（ハイフン）で表示される
 ** %u: HTTP認証によるリクエストしたユーザID（認証していない場合は、"-"）
 ** %t: サーバーがリクエストを受け取った時刻
 ** \"%r\": メソッド、パス、プロトコルなど
 ** %>s: ステータスコード
 ** %b: クライアントに送信されたオブジェクトサイズ（送れなかった時は、"-"）


=== フィールド定義
アクセスログのログフォーマットがわかったので、フィールド名を定義していきたいと思います。
また、この時にタイプも定義しておくとよいです。
てことで、()内にタイプを記載します。

#@#表形式に変更
 * %hは、クライアントIPということで"clientip"(string)
 * %lは、アイデンティティ情報なので、"ident"(string)
 * %uは、認証なので、"auth"(string)
 * %tは、時刻なので"date"(date)
 * \"%r\"は、いくつかに分割したいので、メソッドは、"verb"、パスは、"path"、んでHTTPバージョンは、"httpversion"(一式string)
 * %>sは、ステータスコードなので、"response"(long)
 * %bは、オブジェクトサイズなので、"bytes"(long)


仮に上記がマッピングされると以下のようにいい感じになるはず！

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

=== GrokPatternをつくる

Grok Filterには、@<code>{GrokPattern}（@<href>{https://github.com/elastic/logstash/blob/v1.4.2/patterns/grok-patterns}）
という形であらかじめ正規表現のパターン定義が用意されているので、ふんだんに使っていきたいと思います。
#@#Grok Patternとは、をもうすこし具体的に（参照方法・参考の仕方とか）

ただ、GrokPatternにないものは自分で作成する必要があります！
そこは、次章で説明したいと思いますm(_ _)m

それでは、ここからは各フィールドを見ながらGrokPatternを作成していきたいと思います。
GrokPatternを作成していくには、ログを左から順に攻略していくのが重要です。
これを念頭において進めていきたいと思います。

ちなみにですが、そもそものGrokFilterの書き方とかはひとまず置いておきます！
後ほど、その辺は詳しく書きます。

それではここからは先ほどフィールド定義した順番で解説していきます。

#@# Grok Patternで作成した正規表現をどのようにLogstashのコンフィグに埋め込むのか詳しく
#@# fluentdにもGrok Patternあるのか調べる

=== ClientIP
ClientIPということで、IPアドレスにマッチさせる必要があります。
まずは、IPアドレスにマッチさせるためのGrokPatternがすでにないか、GrokPatternのサイト上で確認します。

#@#具体例あると良い（GrokPatternのどこを参照するか？）

…あるではないか！（茶番劇っぽくてすまそんです）


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


HOSTNAMEに正規表現が記載されていることがわかりますね。
また、IPは、IPv6とIPv4に対応できるように構成されてます。
これも同じ様にサイトをみると正規表現で記載されていることがわかると思います。

IPORHOSTでHOSTNAMEとIPが定義されていることがわかったと思いますが、@<code>{(?:)}と@<code>{|（パイプ）}はなんぞや？と思った人もいると思います。
この@<code>{(?:)}は、文字列をマッチさせたい、かつキャプチャさせたくない場合に使います（キャプチャは使用しないので今回は説明を省略します）。
今回でいう文字列は、@<code>{%{HOSTNAME}}と@<code>{%{IP}に該当する文字列を指します。
また、@<code>{|}は、どちからか一方が一致した方を採用するという意味です。

結果、IPORHOSTは、HOSTNAMEかつ、IPに該当するものをマッチさせる、という設定となっています。

上記を踏まえてGrokPatternを記載すると以下な感じになります。

//list[stage04_list06][IPORHOSTのGrokPattern]{
%{IPORHOST:clientip}
//}

@<img>{stage04-01}にイメージ図を載せましたので参考にしてみてください。

//image[stage04-01][IPアドレスをGrokするイメージ図#01]{
  Grok説明図
//}

それでは、実際にGrokがマッチするかをGrok Constructorを使って確認してみたいと思います。


== Grok Constructorでテスト

#@#Grok Constructorは補足にするか？（要相談）

Grok Constructor（@<href>{http://grokconstructor.appspot.com/do/match}）は、作成したGrokがマッチするかをブラウザベースでテストすることが可能なツールです。
この他にもGrokDebugger（@<href>{https://grokdebug.herokuapp.com/}）やKibanaのX-Packをインストールすることで、KibanaのDevToolsでGrokDebuggerを使ってテストもできます。
であれば、KibanaのDevTools使えよ！というご意見もあるかと思いますが、手軽にGrok Filterのテストを実施したいためGrok Constructorを使用します。
また、個人的に使いやすいというのが大きいですがw

Grok Constructorの使い方を@<img>{stage04-02}で解説したいと思います。

//image[stage04-02][Grok Constructorでテスト#01]{
  Grok Constructor
//}

それでは早速、先ほど作成したGrokPatternがうまい具合にマッチするか試したいと思います。


=== clientip

//image[stage04-03][Grok Constructorでテスト#02]{
  Grok Constructor
//}

想定通り、clientipというフィールドに "5.10.83.30"というIPアドレスがマッチしたことがわかります。
この調子で、他のフィールドに対しても定義していきたいと思います！

=== ident
ユーザ名が付与されるのと@<code>{-}も含めてマッチできるものをGrokPatternで探すと@<code>{USER}というGrokPatternがあるのでこちらを使用します。

//list[stage04_list07][identのGrokPattern]{
%{USER:ident}
//}

先ほどの様に、上記のGrokPatternでGrok Constructorでテストを実施するとIPアドレスが引っかかると思います。
なので、%{IPORHOST:clientip}を含んでテストを実施してみてください。

//image[stage04-04][Grok Constructorでテスト#03]{
  Grok Constructor
//}

=== auth
authもUserと同様の定義で良いので、GrokPatternの@<code>{USER}を使用します。
また、identとauthの間もスペースがあるので@<code>{\s}もしくはスペースを入力する必要があります。
図の記載では\sを¥sで記載してますm(_ _)m

//footnote[stage04-fn01][個人的には、可読性を重視して\sを使用してます]


=== date
次は、時刻ですね！
時刻のフォーマットは、@<code>{[day/month/year:hour:minute:second zone]}です。
これに当てはまるGrokPatternを探していたいと思いますー

以下のGrokPatternが当てはまることがわかります。

//list[stage04_list08][dateのGrokPattern]{
HTTPDATE %{MONTHDAY}/%{MONTH}/%{YEAR}:%{TIME} %{INT}
//}

なので、こちらを使用してGrok Constructorでテストしてみたいと思います。
先ほど作成したGrok Constructorに連ねてきましょー

//image[stage04-05][Grok Constructorでテスト#04]{
  Grok Constructor
//}

あれ？"NOT MATCHED"ですね。。
そうなんです！
じつは、@<code>{%{HTTPDATE}}に該当しない@<code>{[]}があるのです。
なので、以下の図で示している通り、@<code>{[]}を取り除く必要があるのです！
無効化するにはエスケープ（\：バックスラッシュ）を使用します。

//image[stage04-06][IPアドレスをGrokするイメージ図#02]{
  Grok
//}


=== リクエスト
それでは、クライアントからのリクエストについて攻略したいと思いますー。
これは、ダブルクォーテーションの中にひとまとまりされているので、取りたい情報を定義したフィールドにマッチできるようにGrokPatternを作成していきたいと思います。

//list[stage04_list09][リクエストのGrokPattern]{
"GET /test.html HTTP/1.0"
//}

まず、GETですが、GETという文字列以外にもPOSTや、DELETEなどがあります。
なので単純にGETという固定文字でマッチングさせるのでは、あかんのです。
また、GET|PUT|DELETE..etcなどもイケてないですね。。
汎用的にすることと、可読性を意識してマッチングさせたいためです。

ということで、英単語が入るということがわかっているので、\bxxx\b（xxxは何かしらの英単語）に該当するGrokPatternを使用します。

いつも通り、GrokPatternを探すと以下が該当しますね。

//list[stage04_list10][英単語のGrokPattern]{
WORD \b\w+\b
//}

次にパスですが、リクエストによって変動したりするため、柔軟性を求めて以下のNOTSPACEを使用します。
NOTSPACEは、空白文字以外にマッチのため、空白文字が出現するまでマッチします。

//list[stage04_list11][NOTSPACEのGrokPattern#01]{
NOTSPACE \S+
//}

最後のHTTPのバージョンですが、HTTP部分は不要なので取り除くのと、そもそも、HTTPバージョンがはいっていないパターンもあったりします。
そんな時は、@<code>{(?:)?}を利用するこで、このGrokPatternにマッチする時は使うけど、マッチしない時は使わないよ！といった定義ができるのです！（素敵）
これは、便利なので覚えて置いてくださいな！
最後に最短マッチとして、@<code>{%{DATA\}}もパイプで組み込んでます。

#@#Re:VIEWの@<>{}で}が必要なときはバックスラッシュ

//list[stage04_list12][NOTSPACEのGrokPattern#02]{
(?: HTTP/%{NUMBER:httpversion})?|%{DATA:rawrequest})"
//}

てことで、ここまでを@<img>{stage04-07}にまとめましたーヽ(*ﾟдﾟ)ノ

//image[stage04-07][IPアドレスをGrokするイメージ図#03]{
  Grokパワポ
//}


=== response & bytes
ここまできたらあと少し！

responseは、ステータスコードなので、@<code>{NUMBER}を使用します。
また、bytesも同様に@<code>{NUMBER}を使用しますが、オブジェクトが送れなかった場合は、@<code>{-}のため、@<code>{|}で@<code>{-}を追加します。

これで全て整ったので、Grok Constructorでテストしたいと思います。


=== Grok Constructor全体テスト
以下のGrokPatternでテストをしたいと思います。実際は改行しませんが、本文の都合上適宜改行しています。

//list[stage04_list13][最終的なGrokPattern]{
%{IPORHOST:clientip} %{USER:ident} %{USER:auth} \[%{HTTPDATE:date}\]
"(?:%{WORD:verb} %{NOTSPACE:path}(?: HTTP/%{NUMBER:httpversion})?|%{DATA:rawrequest})"
%{NUMBER:response} (?:%{NUMBER:bytes}|-)
//}

//image[stage04-08][Grok Constructorでテスト#04]{
  Grok Constructor
//}

問題なくマッチしましたね！


== logstashを動かしてみる
やっとここでLogstashのconfファイルが登場します。
それでは、confファイルを作成したいと思いますー。

今までは、INPUTとOUTPUTのみでしたが、先ほど作成したGrokPatternを埋め込みたいので、FILTERを追加します。
GrokPatternをFILTERに直接コーディングすることも可能ですが、可読性を意識したいため、GrokPatternをconfファイルとして外出しします。

外出しするため、以下の作業を実施します。

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

次にGrokPatternファイルを作成したので、ログの変換をさせるためとGrokPatternを読み込むためにLogstashのconfファイルに以下を記載します。
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

それでは、実行してみますー

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

おぉ！いい感じにフィールドが抽出できたーヾ(´Д｀)ノｲｴｰｲ

が、しかし、コレでは足りない！
ログのタイムスタンプではなく、ログを取り込んだ時刻になっているので、修正が必要です。
また、グローバルIPがあるんだから、地域情報とマッピングしたいですよね！
ということで、Logstashのconfファイルを修正したいと思いますー

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

 1. ファイルの読み込み位置を指定するためで、Logstash起動前のログも対象としたいため、"biginning"としてます
 2. パターンファイルの読み込み
 3. messageフィールドに格納されている値を”HTTPD_COMMON_LOG”でマッチングします
 4. パターンファイル内でIPアドレスをマッチングさせているclientipフィールドを対象にgeoipフィルタを利用し、地理情報を取得します
 5. Logstashは、ログデータを取り込んだ時間を@timestampに付与するので、dateフィルタを用いることで実際のログデータのタイムスタンプを付与することができます
 6. パターンファイル内のdateフィールドに対して定義したdateパターンとマッチする場合に値を書き換えます
 7. 日付の月が"Oct"になっているため、localeを"en"に指定してます
 8. 変更を変えたいターゲットとして"@timestamp"を指定します
 9. 不要なフィールドをremove_fieldで指定し、削除します（容量を抑えるためと不必要な情報を与えないため）

それでは、修正したconfファイルで再度実行すると以下の感じになります。
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

次は、既に存在しているGrokPatternだけでは取り込めないログをベースに説明していきたいと思います。
それでは、次章でーヽ(*ﾟдﾟ)ノ

== 補足
Grok Constructorで、作成したGrokPatternをテストすることも可能です。
以下の図にあるとおりにテストして頂ければと思います。

//image[stage04-10][Grok Constructorでテスト#05]{
  Grok Constructor
//}
