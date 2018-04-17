= 比較その2：コンフィグの形式
fluentdとLogstashのコンフィグ形式は、かなり違いがあります。
ここではデータの読み取り・加工・出力に分けて、コンフィグの比較をしてみたいと思います。
なお、今回はjsonデータをcsvに加工・出力する、という事例に沿ってコンフィグを記載しています。


ちなみに、jsonファイルは@<href>{http://www.databasetestdata.com/}を利用して作成しました。
json,csv,xmlから拡張子を選択しデータの中身を項目を指定すれば、自動でテストデータが生成されます。
本の表記の都合で改行していますが、実際に使用したデータは改行はありません。

//list[test_json][今回使用したjsonファイル]{
{"Full Name": "Sheldon Tillman","Country": "Peru",
"Created At": "1980-11-13T17:37:36.702Z",
"Id": 0,"Email": "Carolanne_Kub@emmalee.name"}
//}

この章で扱う入力データの例は、全て@<list>{test_json}を使用しています。

== fluentd

=== コンフィグの構造
fluentdのコンフィグ（td-agentを用いてインストールした場合は@<code>{td-agnet.conf}）の構造は、次のようになっています。
また、コンフィグは@<code>{/etc/td-agent/td-agnet.conf}にデフォルトで配置されているので、こちらを編集していきます。


ただし、fluentdは@<code>{source}→@<code>{filter}→@<code>{match}の順に処理を行います。
コンフィグの記述位置と処理の順序は関係ありません。

=== データの読み取り部（source）
fluentdがどのデータを読み取るのかをこのディレクティブで指定します。データの入力元は複数指定することが可能です。
@<code>{<source>}と@<code>{</source>}の間に設定を記述します。この書式は他のディレクティブでも同様です。

//list[fluentd_source][source部分の実装例]{
<source>
  @type tail
  path /var/log/json/*.json
  pos_file /var/log/td-agent/tmp/json.log.pos
  format json
  tag json
</source>
//}

@<code>{@type}は使用するプラグイン名です。今回はサーバー内のjsonファイルを取得することが目的ですので、
@<code>{tail}プラグインを選択しました。@<code>{tail}プラグインは、Linuxコマンドに例えると@<code>{tail -F}と同じような働きをします@<fn>{fluentd_tail}。
具体的には指定したファイルを監視し、ファイルが更新されるとfluentdがデータを取得する、という動きをします。

//footnote[fluentd_tail][https://docs.fluentd.org/v0.12/articles/in_tail]

@<code>{path}はどのファイルを取得対象とするか設定する箇所です。実装例では@<code>{/var/log/json}に存在するjsonファイルを全て取得する設定になっています。
@<code>{path}は必須設定です。

@<code>{pos_file}はファイルをどこまで読み取ったか記録しておく@<code>{.pos}ファイルを
どこに配置か設定します。@<code>{path}と違って任意設定ですが、設定することが推奨されています。

@<code>{format}は指定した形式にデータを整形します。こちらは必須設定です。

@<code>{tag}は取得したデータに付与するデータの名称を設定します。このtagを用いて
データの送信先の振り分けなどを行うことができます。こちらも必須設定です。

=== データの送付部（match）
処理が終わったデータをどこに送付するか指定するディレクティブです。
プラグインを複数記述すれば、複数の出力先へデータを送付できます。

//list[fluentd_match][match部分の実装例]{
<match>
  @type file
  path /var/log/csv/test.csv
  format csv
  fields Full Name,Country,Created At,Id,Email
</match>
//}

@<code>{type}は使用するプラグイン名です。取得したデータをcsvファイルにして出力したいので
@<code>{file}プラグインを指定しています。出力形式は@<code>{format}で指定します。

@<code>{path}はファイルの出力先を指定します。例では@<code>{/var/log/csv}に@<code>{test.csv}
ファイルを出力する設定となっています。

@<code>{format}をcsvにした場合、@<code>{fields}を設定する必要があります。
ここに設定した文字列がcsvの列となります。

=== コンフィグ例と出力結果
今回の実装例である、「jsonファイルを取得し、csvに加工する」ためfluentdのコンフィグと
出力例を記載します。入力データは@<list>{test_json}を使用しています。

//list[fluentd_example][fluentdのコンフィグ例まとめ]{
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
//}

//list[fluentd_output][test.csvの出力例（fluentd）]{
"Sheldon Tillman","Peru","1980-11-13T17:37:36.702Z",
"0","Carolanne_Kub@emmalee.name"
//}

@<code>{""}を出力データから削除したい場合、@<code>{<match>}部分に@<code>{force_quotes false}の設定を追加します。


== Logstash

=== コンフィグの構造
対するLogstashのコンフィグ（@<code>{logstash.conf}）の構造は、次のようになっています。
また、コンフィグは@<code>{/etc/logstash/conf.d}配下に置きます。名称の最後を@<code>{.conf}にして配置します。

特徴として、コンフィグにはデータ処理の内容しか記載しないこと・各データに対するタグ付けが任意であることが挙げられます。
Logstash自体のソフトウェアに関する動作設定は、全て@<code>{/etc}配下にある設定ファイル（@<code>{logstash.yml}）で行います。
ここでは@<code>{logstash.conf}のみに焦点を絞って中身をみていきます。

=== データの読み取り部（input）
Logstashがどのデータを読み取るのか、この部分で指定します。必須設定項目はプラグインごとに異なり、共通のものはありません。
取得したいデータに合わせてどのプラグインを使用するか決定し、使用方法を確認する必要があります。

今回はサーバー内に配置されたjsonファイルを取得するため、@<code>{file}プラグインを使用します（@<href>{https://www.elastic.co/guide/en/logstash/6.0/plugins-inputs-file.html}）。

//list[input_json_logstash][inputプラグインの実装例]{
input {
  file {
  	path => "/var/log/json/*.json"
  }
}
//}

@<code>{file}はプラグイン名です。@<code>{path}はどこからデータを取得するか記載しています。この場合@<code>{/var/log/json}から
jsonファイルを全て取得する、という設定になっています。


=== データの加工部（filter）
読み取ったデータをどのように加工・整形するのか指定します@<fn>{logstash_filter}。

//footnote[logstash_filter][ここでの加工・整形とは、データの新規追加・削除・データ型の変更・データ分割などを指します。]

プラグインごとにできることが異なりますので、プラグイン同士を組み合わせてコンフィグを作成します。
基本は取得するデータ形式に応じたプラグインでデータをいい感じに分割し、その後データを煮るなり焼くなりして加工する、という感じです@<fn>{filter_make}。

//footnote[filter_make][こういうの、がらがらぽんってできると楽だと思うんですけど世の中厳しい。]

例えば、今回のようにjsonを取得した場合、@<code>{json}プラグインを使用してjsonを分割します。

//list[filter_json_logstash][filterプラグインの実装例]{
  filter {
    json {
     	source => "message"
   }
  }
//}

@<code>{json}はプラグイン名称です。オプション@<code>{source}はどのfield内のデータを加工するか指定する部分です。
取得したデータはデフォルトだと@<code>{message}というfieldに入るので、@<code>{message}内のデータを加工する
指定を行なっています。

=== データの送付部（output）
処理が終わったデータをどこに送付するか指定するプラグインです。データの送付以外にも、別フォーマットへの加工が可能です。
例えばjsonで取得したデータをcsvに変換して保存、といったことができます。
また、複数の宛先にデータを送付したい場合、プラグインを複数記述することで実現可能です。

//list[output_json_logstash][outputプラグインの実装例]{
output{
  csv {
    fields => [
      "Full Name", "Country", "Created At", "Id", "Email"
    ]
    path => "/var/log/csv/test.csv"
  }
}
//}

@<code>{csv}はプラグイン名称です。このプラグインを使うと、データをcsvに変換できます。
@<code>{fields}で指定した文字がcsvの列にあたります。列に一致するデータが見つからない場合、csvは空欄になります。
@<code>{path}は生成したcsvの出力場所を指定します。日付方法をファイル名にすることも可能です。


=== コンフィグ例と出力結果
今まで紹介した、Logstashのコンフィグと出力結果例を記載します。

//list[Logstash_example][Logstashのコンフィグ例まとめ]{
input {
  file {
  	path => "/var/log/json/*.json"
    }
}
filter {
  json {
  	source => "message"
 }
}
output{
  csv {
    fields => [
      "Full Name", "Country", "Created At", "Id", "Email"
    ]
    path => "/var/log/csv/test.csv"
  }
}
//}

入力データは@<list>{test_json}を参照してください。

//list[Logstash_output][test.csvの出力例（Logstash）]{
Sheldon Tillman,Peru,1980-11-13T17:37:36.702Z,
0,Carolanne_Kub@emmalee.name
//}

jsonデータがcsv形式に加工されていることがわかります。

出力結果だけ比較すると、fluentdとLogstashで全く変わりないことがわかります。
しかし、どの設定でデータを分割するかや、コンフィグに対する考え方が全く違うようですね。
Logstashはデータの流れに沿ってコンフィグを記載するのに対し、fluentdはソフトウェアの動作も含めて
まとめてコンフィグを書いているような印象を受けます。
