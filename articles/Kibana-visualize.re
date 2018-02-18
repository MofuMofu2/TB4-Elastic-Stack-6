= はーいい感じにグラフ作りたいマン

#@#タイトルは適当です

gitコマンドを利用して出力したgitログです。

//list[kibana01-list01][Gitコミットログの出力]{
git log
//}


//cmd{
$ git log
commit 18372016d051ad313f581244378470999c81d788
Author: MofuMofu2 <froakie002@gmail.com>
Date:   Sun Feb 18 16:07:47 2018 +0900

   [add] 本文がないとビルドがこけるので、テストファイルを追加

commit b4b18e907d5d9b2f2a93aed7cea798bd3f404232
Author: MofuMofu2 <froakie002@gmail.com>
Date:   Sun Feb 18 16:00:23 2018 +0900

   [add] 著者リストを追加

commit 14b98b2d3c7e5395e2e706f57ec546791b965df6
Author: MofuMofu2 <froakie002@gmail.com>
Date:   Sun Feb 18 15:57:54 2018 +0900
//}

一行で出力する場合は@<code>{--oneline}オプションをつけます。

//cmd{
git log --oneline
a5f089c [add] Kibanaの章を追加
1837201 [add] 本文がないとビルドがこけるので、テストファイルを追加
b4b18e9 [add] 著者リストを追加
//}

今回欲しい情報は次の通り

* ハッシュ値（コミットの特定の為に必要）
* Author（だれがコミットしたのか特定する為に必要）
* Authorのメールアドレス（なくてもいいけど）
* コミット時刻（いつコミットしたのかを特定する為に必要）
* コミットメッセージ（概要しりたいじゃん？）

コミットの時刻は@<code>{iso}形式で出力しておいた方が楽そう。最低限分まではみたい。


//cmd{
$ git log  --oneline --pretty=format:"%h, %an, %ae, %ad, %s " --date="iso"
e9c1356, MofuMofu2, froakie002@gmail.com, 2018-02-18 18:00:39 +0900, [add] git logを出力する
a5f089c, MofuMofu2, froakie002@gmail.com, 2018-02-18 17:40:29 +0900, [add] Kibanaの章を追加
1837201, MofuMofu2, froakie002@gmail.com, 2018-02-18 16:07:47 +0900, [add] 本文がないとビルドがこけるので、テストファイルを追加
b4b18e9, MofuMofu2, froakie002@gmail.com, 2018-02-18 16:00:23 +0900, [add] 著者リストを追加
14b98b2, MofuMofu2, froakie002@gmail.com, 2018-02-18 15:57:54 +0900, [fix] ファイル名をwercker出力名に合わせる
c201d78, MofuMofu2, froakie002@gmail.com, 2018-02-18 15:56:35 +0900, ひつようなファイルをコミットだよ
4e62dae, MofuMofu2, froakie002@gmail.com, 2018-02-18 15:30:47 +0900, [fix] reviewファイルの前に半角ハイフンがなかったので修正
//}

これをjson形式で出力し、Elasticsearchに投入する。

@<code>{git-log-to-json}というnpmパッケージを利用すると@<href>{https://www.npmjs.com/package/git-log-to-json}、Node.jsを
利用してgit logをjson形式で出力できるようです。今回は本題から外れるので扱いませんが、またどこかで記事を公開したいですねー。
