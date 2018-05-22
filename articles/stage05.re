= "Grok芸人"への道
== 今度は何を取り込む？
前章でApacheのアクセスログを取り込めるようになりました。
そこで既存のGrokPatternだけではどうにもならない系のログを対象にGrokしていきたいと思います。

ちなみにですが、GrokPattenはJava、bind、Redisなど様々なものが用意されてます。

また、FireWallという括りでCiscoのASAのGrokPatternが用意されているものもあります。
ただ、すべてがまかなえてるかと言うとまかなえてないです。

なので、今回はCiscoのファイアウォール製品であるASAのログを取り込みたいと思います！
やっぱり企業を守っているファイアウォールがどんなログを出しているか気になりますよね！？（薄っぺらいw）
あ、でも使えるGrokPatternは積極的に使います！
当たり前ですが、"あるものは使う！"、"ないものは作る！"という心得でいきましょー


といことで今回は以下のログを対象にしたいと思います。
IPアドレスは、適当なプライベートIPアドレスを割り当てています。

//list[stage05-list01][Cisco ASAのログ]{
Jun 20 10:21:34 ASA-01 : %ASA-6-606001: ASDM session number 0
 from 192.168.1.254 started
Jun 20 11:21:34 ASA-01 : %ASA-6-606002: ASDM session number 0
 from 192.168.1.254 ended
//}



いつも通りに以下のログ取り込みフローで進めたいと思います！


//list[stage05_list02][ログの取り込みフロー（再掲）]{
1. ログフォーマットを調べる
2. フィールド定義
3. GrokPatternをつくる
4. Grok Constructorでテスト
5. Logstashを動かしてみる
//}


=== ログフォーマットを調べる
まずは、ログフォーマットを調べる！
Ciscoさんは丁寧にログフォーマットを掲載してます（URL:@<href>{https://www.cisco.com/c/en/us/td/docs/security/asa/syslog/b_syslog.html}）。

…よく見るとわかりますが、数が多いw

まぁ、Ciscoって世界最大ですからねー(*ﾟ∀ﾟ)/ｱｯｹﾗｶﾝ

まず該当するログフォーマットを探す方法ですが、以下のログに"%ASA-6-606001"という記載がありますので、このイベントNo.の"606001"で検索することができます。

このログフォーマットは以下のようになっています。

//list[stage05-list03][ASAのログフォーマット]{
%ASA-6-606001: ASDM session number number from IP_address started
%ASA-6-606002: ASDM session number number from IP_address ended
//}



ASDM（Webベースの管理インターフェースを提供するツール）のセッションを開始した時と終了した時に出力するログですね。



=== フィールド定義
ではでは、フィールド定義ですが、ログの左から順にやっていきます。
先ほど見たログフォーマットには、タイムスタンプとASA-01というのがなかったと思います。
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
例えば、@<code>{ASDM session number}というメッセージに対して意味はなく、そのセッションナンバーが知りたいのです。
そのため、フィールド名に@<code>{ASDM session number}とし、値としては取り込まないようにします。
その他の@<code>{from}も同様で、どこからのIPアドレスかを知りたいため、fromを取り除き、@<code>{src_ip}（ソースIP）というフィールドにIPアドレスを値として取り込みたいと思います。

2つ目のログ（以下に記載）ですが、最後の@<code>{ended}しか変わらないということがわかります。
なので、先ほどのフィールド定義をそのまま使用するので割愛します。


== GrokPatternをつくる
それでは、GrokPatternを作っていきます。


=== 共通部分
タイプスタンプとホスト名、イベントIDはすべてのログに入るメッセージのため、共通部分とします。
それでは、タイムスタンプとホスト名、イベントIDに取り掛かりたいと思います！

タイムスタンプは、GrokPatternに @<code>{CISCOTIMESTAMP}を使用します。

//list[stage05_list04][CISCOTIMESTAMPのGrokPattern]{
CISCOTIMESTAMP %{MONTH} +%{MONTHDAY}(?: %{YEAR})? %{TIME}
//}


また、ホスト名は、ユーザーが自由に付与する名前のため、柔軟性を求めて@<code>{NOTSPACE}を使用します。
また、先頭にスペースが必ず入るので@<code>{\s}を入れます。


//list[stage05_list05][HOSTNAMEのGrokPattern]{
HOSTNAME \s%{NOTSPACE:hostname}
//}


イベントIDは、GrokPatternに用意されていないので、自分で作成します。
自分でGrokPatternを作成する場合は以下のように作成します。

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



=== ASDMセッションNo
フィールド定義で記載した通りですが、ASDMセッションNo.をフィールドとし、値が取得できればよいわけです。
なので、以下のようになります。

//list[stage05_list09][ASDMセッションNoのGrokPattern]{
ASDM session number(?<ASDM-sesion-number>\s[0-9]+)
//}


これも見てわかる通り、CustomPatternで作成しています。
一つ一つみていくと@<code>{(?)}の外に@<code>{ASDM session nubber}がありますね。
これは、@<code>{ASDM session nubber}をマッチしても値は取得したくない場合に使うやり方です。
なので、隣の@<code>{(?<ASDM-session-number>\s[0-9]+)}というCustomPatternで取得した値が
@<code>{ASDM-session-number}というフィールドに入ります。
正規表現部分は、@<code>{\s}のスペースと@<code>{0-9}の数字が複数並んでも対応できるように@<code>{+}を使用してます。

最終的に先頭の@<code>{:}とスペースも含むので以下な感じになります。

//list[stage05_list10][ASDMセッションNoのGrokPattern（完成版）]{
:\sASDM session number(?<ASDM-session-number>\s[0-9]+)
//}



=== ソースIPアドレス
これはApacheのアクセスログでやったことと一緒ですね。
IPアドレスのGrokPatternのように他にも確立されているものは積極的に使っていきましょう。
あるものは使う、なければCustomPattern！

* from 192.168.1.254

これは、フィールド定義で説明したようにソースIPなので、GrokPatternの@<code>{IP}を使用し、不要な部分を取り除く必要があります。
スペースと@<code>{from}が不要なのでGrokPatternの外側に出しますが、一つの文字列とするため()で囲います。

//list[stage05_list11][ソースIPアドレスのGrokPattern]{
(\sfrom\s%{IP:src_ip})
//}

=== ステータス
最後はセッションステータスを表す@<code>{started}ですね。
これは、CustomPatternで対応します。
先ほどのソースIPとの間にスペースがあるので@<code>{\s}を入れます。
また、@<code>{started}は文字列なので@<code>{\b}を入れて以下な感じです。

//list[stage05_list12][セッションステータスのGrokPattern]{
\s(?<session>\bstarted)
//}


ただ、もう一つのイベントID"606002"ですが、ステータスがendedしか変わりません。
なので、先ほどのステータスに"started""ended"のどちらかを選択できるようにします。

//list[stage05_list13][ステータスの選択を可能にする]{
\s(?<session>\bstarted|\bended)
//}


@<code>{|}を入れることで選択できるようになります。
これで整ったので、GrokConstructorでテストをしてみたいと思います。


== Grok Constructorでテスト
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


実行結果は以下です！

//image[stage05-01][ASA Grok Constructor結果#01]{
  Grok Constructor
//}


== logstashを動かしてみる
ここまできましたね！

ここまできたら後少し！ということでApacheのアクセスログの時と同様にconfファイルを作成していきたいと思いますー

今回もパターンファイルに外出ししたいと思います！

=== パターンファイル
タイムスタンプやらホスト名、イベントIDそしてイベントメッセージのGrokPatternをパターンファイルに定義します。
GrokPatternの"CISCOFW606001"に"606002"も含んでいるのですが、文字数が長くなるのが個人的に嫌なため"606001"に集約してます。
なので、含んでいることがわかりにくいと思う方は変更しても問題ないです。

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


=== logstash.conf
ここまできましたね！

Logstashのconfファイル作成して実行して動いたら勝ちパターン！

Apacheの時と同様に作成してみたのが以下です！ファイル名はasa.confとして保存しました。

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

実行結果を以下に記載しますー

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

想定通りにログを抽出できましたね！

GrokPatternにあるものは積極的に使用し、ないものはCustomPatternで作る！といったことを学習できたのではないでしょうか。
CiscoのASAを今回取り上げましたが、この他のログも同様に対応していくことが可能です。
更に"Grok芸人"になるためにも様々なログにトライしてみてください！

それでは本章はこれでおわりですー

//indepimage[stage05-03][エサを求めて群がるペンギン]
