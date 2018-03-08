~/Elastic-Stack/logstash-6.2.2 $ ls -al
total 128
drwxr-xr-x@ 17 mallow  staff    544  3  7 13:07 .
drwxr-xr-x   6 mallow  staff    192  3  7 11:28 ..
-rw-r--r--@  1 mallow  staff   2276  2 16 20:55 CONTRIBUTORS
-rw-r--r--@  1 mallow  staff   3843  2 16 20:55 Gemfile
-rw-r--r--@  1 mallow  staff  21148  2 16 20:55 Gemfile.lock
-rw-r--r--@  1 mallow  staff    589  2 16 20:55 LICENSE
-rw-rw-r--@  1 mallow  staff  28122  2 16 20:55 NOTICE.TXT
drwxr-xr-x@ 17 mallow  staff    544  3  7 10:50 bin
drwxr-xr-x@  8 mallow  staff    256  3  7 11:16 config
drwxr-xr-x@  6 mallow  staff    192  3  7 13:07 data
drwxr-xr-x@  6 mallow  staff    192  3  7 10:50 lib
drwxr-xr-x   4 mallow  staff    128  3  7 13:07 logs
drwxr-xr-x@  6 mallow  staff    192  3  7 10:50 logstash-core
drwxr-xr-x@  4 mallow  staff    128  3  7 10:50 logstash-core-plugin-api
drwxr-xr-x@  5 mallow  staff    160  3  7 10:50 modules
drwxr-xr-x@  3 mallow  staff     96  3  7 10:50 tools
drwxr-xr-x@  4 mallow  staff    128  3  7 10:51 vendor
~/Elastic-Stack/logstash-6.2.2 $ clear
~/Elastic-Stack/logstash-6.2.2 $ bin/logstash -f config/logstash.conf
Sending Logstash's logs to /Users/mallow/Elastic-Stack/logstash-6.2.2/logs which is now configured via log4j2.properties
[2018-03-07T13:08:26,103][INFO ][logstash.modules.scaffold] Initializing module {:module_name=>"netflow", :directory=>"/Users/mallow/Elastic-Stack/logstash-6.2.2/modules/netflow/configuration"}
[2018-03-07T13:08:26,129][INFO ][logstash.modules.scaffold] Initializing module {:module_name=>"fb_apache", :directory=>"/Users/mallow/Elastic-Stack/logstash-6.2.2/modules/fb_apache/configuration"}
[2018-03-07T13:08:26,417][WARN ][logstash.config.source.multilocal] Ignoring the 'pipelines.yml' file because modules or command line options are specified
[2018-03-07T13:08:27,150][INFO ][logstash.runner          ] Starting Logstash {"logstash.version"=>"6.2.2"}
[2018-03-07T13:08:27,321][INFO ][logstash.config.source.local.configpathloader] No config files found in path {:path=>"/Users/mallow/Elastic-Stack/logstash-6.2.2/config/logstash.conf"}
[2018-03-07T13:08:27,342][ERROR][logstash.config.sourceloader] No configuration found in the configured sources.
[2018-03-07T13:08:27,631][INFO ][logstash.agent           ] Successfully started Logstash API endpoint {:port=>9600}


~/Elastic-Stack/filebeat-6.2.2-darwin-x86_64 $ sudo ./filebeat -e -c filebeat.yml
Password:
Exiting: error loading config file: config file ("filebeat.yml") must be owned by the beat user (uid=0) or root
~/Elastic-Stack/filebeat-6.2.2-darwin-x86_64 $ ls -al
total 51064
drwxr-xr-x@ 13 mallow  staff       416  2 17 03:47 .
drwxr-xr-x   6 mallow  staff       192  3  7 11:28 ..
-rw-r--r--@  1 mallow  staff        41  2 17 03:46 .build_hash.txt
-rw-r--r--@  1 mallow  staff       583  2 17 03:46 LICENSE.txt
-rw-r--r--@  1 mallow  staff    198236  2 17 03:46 NOTICE.txt
-rw-r--r--@  1 mallow  staff       802  2 17 03:46 README.md
-rw-r--r--@  1 mallow  staff     44384  2 17 03:47 fields.yml
-rwxr-xr-x@  1 mallow  staff  25825132  2 17 03:47 filebeat
-rw-r-----@  1 mallow  staff     52193  2 17 03:47 filebeat.reference.yml
-rw-------@  1 mallow  staff      7330  3  7 11:40 filebeat.yml
drwxr-xr-x@  4 mallow  staff       128  2 17 03:45 kibana
drwxr-xr-x@ 14 mallow  staff       448  2 17 03:42 module
drwxr-xr-x@ 14 mallow  staff       448  2 17 03:46 modules.d

たぶん所有権の問題だろうな

~/Elastic-Stack/filebeat-6.2.2-darwin-x86_64 $ sudo chown root filebeat.yml

~/Elastic-Stack/filebeat-6.2.2-darwin-x86_64 $ ls -al
total 51064
drwxr-xr-x@ 13 mallow  staff       416  2 17 03:47 .
drwxr-xr-x   6 mallow  staff       192  3  7 11:28 ..
-rw-r--r--@  1 mallow  staff        41  2 17 03:46 .build_hash.txt
-rw-r--r--@  1 mallow  staff       583  2 17 03:46 LICENSE.txt
-rw-r--r--@  1 mallow  staff    198236  2 17 03:46 NOTICE.txt
-rw-r--r--@  1 mallow  staff       802  2 17 03:46 README.md
-rw-r--r--@  1 mallow  staff     44384  2 17 03:47 fields.yml
-rwxr-xr-x@  1 mallow  staff  25825132  2 17 03:47 filebeat
-rw-r-----@  1 mallow  staff     52193  2 17 03:47 filebeat.reference.yml
-rw-------   1 root    staff      7330  3  7 11:40 filebeat.yml
drwxr-xr-x@  4 mallow  staff       128  2 17 03:45 kibana
drwxr-xr-x@ 14 mallow  staff       448  2 17 03:42 module
drwxr-xr-x@ 14 mallow  staff       448  2 17 03:46 modules.d

~/Elastic-Stack/filebeat-6.2.2-darwin-x86_64 $ sudo ./filebeat -e -c filebeat.yml
2018-03-07T13:15:14.256+0900	INFO	instance/beat.go:468	Home path: [/Users/mallow/Elastic-Stack/filebeat-6.2.2-darwin-x86_64] Config path: [/Users/mallow/Elastic-Stack/filebeat-6.2.2-darwin-x86_64] Data path: [/Users/mallow/Elastic-Stack/filebeat-6.2.2-darwin-x86_64/data] Logs path: [/Users/mallow/Elastic-Stack/filebeat-6.2.2-darwin-x86_64/logs]
2018-03-07T13:15:14.257+0900	INFO	instance/beat.go:475	Beat UUID: d6eb23e4-0575-4601-b51c-d036593964c0
2018-03-07T13:15:14.257+0900	INFO	instance/beat.go:213	Setup Beat: filebeat; Version: 6.2.2
2018-03-07T13:15:14.258+0900	INFO	pipeline/module.go:76	Beat name: ishiiaoi-no-MacBook-Pro.local
2018-03-07T13:15:14.261+0900	INFO	instance/beat.go:301	filebeat start running.
2018-03-07T13:15:14.262+0900	INFO	[monitoring]	log/log.go:97	Starting metrics logging every 30s
2018-03-07T13:15:14.262+0900	INFO	registrar/registrar.go:71	No registry file found under: /Users/mallow/Elastic-Stack/filebeat-6.2.2-darwin-x86_64/data/registry. Creating a new registry file.
2018-03-07T13:15:14.263+0900	INFO	registrar/registrar.go:108	Loading registrar data from /Users/mallow/Elastic-Stack/filebeat-6.2.2-darwin-x86_64/data/registry
2018-03-07T13:15:14.263+0900	INFO	registrar/registrar.go:119	States Loaded from registrar: 0
2018-03-07T13:15:14.263+0900	WARN	beater/filebeat.go:261	Filebeat is unable to load the Ingest Node pipelines for the configured modules because the Elasticsearch output is not configured/enabled. If you have already loaded the Ingest Node pipelines or are using Logstash pipelines, you can ignore this warning.
2018-03-07T13:15:14.263+0900	INFO	crawler/crawler.go:48	Loading Prospectors: 1
2018-03-07T13:15:14.264+0900	INFO	crawler/crawler.go:82	Loading and starting Prospectors completed. Enabled prospectors: 0
2018-03-07T13:15:14.264+0900	INFO	cfgfile/reload.go:127	Config reloader started
2018-03-07T13:15:14.264+0900	INFO	cfgfile/reload.go:219	Loading of config files completed.

https://www.elastic.co/guide/en/beats/libbeat/5.3/config-file-permissions.html

やっぱりなあ


日本語がパスに入ってると落ちる

~/Elastic-Stack/logstash-6.2.2 $ bin/logstash -f config/conf.d/gitlog-logstash.conf
Sending Logstash's logs to /Users/mallow/Elastic-Stack/logstash-6.2.2/logs which is now configured via log4j2.properties
[2018-03-07T13:27:16,612][INFO ][logstash.modules.scaffold] Initializing module {:module_name=>"netflow", :directory=>"/Users/mallow/Elastic-Stack/logstash-6.2.2/modules/netflow/configuration"}
[2018-03-07T13:27:16,641][INFO ][logstash.modules.scaffold] Initializing module {:module_name=>"fb_apache", :directory=>"/Users/mallow/Elastic-Stack/logstash-6.2.2/modules/fb_apache/configuration"}
[2018-03-07T13:27:17,115][WARN ][logstash.config.source.multilocal] Ignoring the 'pipelines.yml' file because modules or command line options are specified
[2018-03-07T13:27:18,091][INFO ][logstash.runner          ] Starting Logstash {"logstash.version"=>"6.2.2"}
[2018-03-07T13:27:18,659][INFO ][logstash.agent           ] Successfully started Logstash API endpoint {:port=>9600}
[2018-03-07T13:27:21,830][INFO ][logstash.pipeline        ] Starting pipeline {:pipeline_id=>"main", "pipeline.workers"=>4, "pipeline.batch.size"=>125, "pipeline.batch.delay"=>50}
[2018-03-07T13:27:22,490][ERROR][logstash.pipeline        ] Error registering plugin {:pipeline_id=>"main", :plugin=>"<LogStash::Inputs::File path=>[\"Users/mallow/Google ドライブ/TB4-Elastic-Stack-6/articles/log/*.json\"], tags=>[\"git-log\"], id=>\"6838e9360bc5a7e28c5b12545eccd1e136fc0b0c3ebd9d68c225a46edc147a31\", enable_metric=>true, codec=><LogStash::Codecs::Plain id=>\"plain_5ad3ef0e-7c73-49af-b904-90ff27b9cd2e\", enable_metric=>true, charset=>\"UTF-8\">, stat_interval=>1, discover_interval=>15, sincedb_write_interval=>15, start_position=>\"end\", delimiter=>\"\\n\", close_older=>3600>", :error=>"File paths must be absolute, relative path specified: Users/mallow/Google ドライブ/TB4-Elastic-Stack-6/articles/log/*.json", :thread=>"#<Thread:0x1c576080 run>"}
[2018-03-07T13:27:22,670][ERROR][logstash.pipeline        ] Pipeline aborted due to error {:pipeline_id=>"main", :exception=>#<ArgumentError: File paths must be absolute, relative path specified: Users/mallow/Google ããã©ã¤ãã/TB4-Elastic-Stack-6/articles/log/*.json>, :backtrace=>["/Users/mallow/Elastic-Stack/logstash-6.2.2/vendor/bundle/jruby/2.3.0/gems/logstash-input-file-4.0.3/lib/logstash/inputs/file.rb:187:in `block in register'", "org/jruby/RubyArray.java:1734:in `each'", "/Users/mallow/Elastic-Stack/logstash-6.2.2/vendor/bundle/jruby/2.3.0/gems/logstash-input-file-4.0.3/lib/logstash/inputs/file.rb:185:in `register'", "/Users/mallow/Elastic-Stack/logstash-6.2.2/logstash-core/lib/logstash/pipeline.rb:341:in `register_plugin'", "/Users/mallow/Elastic-Stack/logstash-6.2.2/logstash-core/lib/logstash/pipeline.rb:352:in `block in register_plugins'", "org/jruby/RubyArray.java:1734:in `each'", "/Users/mallow/Elastic-Stack/logstash-6.2.2/logstash-core/lib/logstash/pipeline.rb:352:in `register_plugins'", "/Users/mallow/Elastic-Stack/logstash-6.2.2/logstash-core/lib/logstash/pipeline.rb:502:in `start_inputs'", "/Users/mallow/Elastic-Stack/logstash-6.2.2/logstash-core/lib/logstash/pipeline.rb:393:in `start_workers'", "/Users/mallow/Elastic-Stack/logstash-6.2.2/logstash-core/lib/logstash/pipeline.rb:289:in `run'", "/Users/mallow/Elastic-Stack/logstash-6.2.2/logstash-core/lib/logstash/pipeline.rb:249:in `block in start'"], :thread=>"#<Thread:0x1c576080 run>"}
[2018-03-07T13:27:22,783][ERROR][logstash.agent           ] Failed to execute action {:id=>:main, :action_type=>LogStash::ConvergeResult::FailedAction, :message=>"Could not execute action: LogStash::PipelineAction::Create/pipeline_id:main, action_result: false", :backtrace=>nil}
~/Elastic-Stack/logstash-6.2.2 $

ながすぎんだよ


なんだこれ
[2018-03-07T13:50:47,620][WARN ][logstash.filters.json    ] Parsed JSON object/hash requires a target configuration option {:source=>"message", :raw=>""}
2018-03-07T04:50:47.493Z ishiiaoi-no-MacBook-Pro.local
2018-03-07T04:50:47.494Z ishiiaoi-no-MacBook-Pro.local  {"commit_hash":"cc97e0a","author_name":"MofuMofu2","author_email":"froakie002@gmail.com","author_date":"2018-03-07 13:11:30 +0900","subject":"[fix] gitコンフィグでちゃんとしたjsonが出力できるように調整"}


~/Elastic-Stack/logstash-6.2.2 $ bin/logstash -f config/conf.d/gitlog-logstash.conf
Sending Logstash's logs to /Users/mallow/Elastic-Stack/logstash-6.2.2/logs which is now configured via log4j2.properties
[2018-03-07T16:04:29,123][INFO ][logstash.modules.scaffold] Initializing module {:module_name=>"netflow", :directory=>"/Users/mallow/Elastic-Stack/logstash-6.2.2/modules/netflow/configuration"}
[2018-03-07T16:04:29,152][INFO ][logstash.modules.scaffold] Initializing module {:module_name=>"fb_apache", :directory=>"/Users/mallow/Elastic-Stack/logstash-6.2.2/modules/fb_apache/configuration"}
[2018-03-07T16:04:29,559][WARN ][logstash.config.source.multilocal] Ignoring the 'pipelines.yml' file because modules or command line options are specified
[2018-03-07T16:04:30,443][INFO ][logstash.runner          ] Starting Logstash {"logstash.version"=>"6.2.2"}
[2018-03-07T16:04:31,157][INFO ][logstash.agent           ] Successfully started Logstash API endpoint {:port=>9600}
[2018-03-07T16:04:35,785][INFO ][logstash.pipeline        ] Starting pipeline {:pipeline_id=>"main", "pipeline.workers"=>4, "pipeline.batch.size"=>125, "pipeline.batch.delay"=>50}
[2018-03-07T16:04:36,310][INFO ][logstash.pipeline        ] Pipeline started succesfully {:pipeline_id=>"main", :thread=>"#<Thread:0x73d5787a run>"}
[2018-03-07T16:04:36,529][INFO ][logstash.agent           ] Pipelines running {:count=>1, :pipelines=>["main"]}



//cmd{
input {
		file {
			path => "/Users/mallow/log/*.json"
			tags => "git-log"
		}
}

# filter {
# 	json {
# 		source => "message"
#
# 	}
# }

output {
	stdout { codec => rubydebug }
}

//}


{
       "message" => " {\"commit_hash\":\"cc97e0a\",\"author_name\":\"MofuMofu2\",\"author_email\":\"froakie002@gmail.com\",\"author_date\":\"2018-03-07 13:11:30 +0900\",\"subject\":\"[fix] gitコンフィグでちゃんとしたjsonが出力できるように調整\"}",
          "path" => "/Users/mallow/log/gitLog.json",
      "@version" => "1",
          "tags" => [
        [0] "git-log"
    ],
    "@timestamp" => 2018-03-07T07:04:47.011Z,
          "host" => "ishiiaoi-no-MacBook-Pro.local"
}
{
       "message" => " {\"commit_hash\":\"cc97e0a\",\"author_name\":\"MofuMofu2\",\"author_email\":\"froakie002@gmail.com\",\"author_date\":\"2018-03-07 13:11:30 +0900\",\"subject\":\"[fix] gitコンフィグでちゃんとしたjsonが出力できるように調整\"}",
          "path" => "/Users/mallow/log/gitLog.json",
      "@version" => "1",
          "tags" => [
        [0] "git-log"
    ],
    "@timestamp" => 2018-03-07T07:04:47.084Z,
          "host" => "ishiiaoi-no-MacBook-Pro.local"
}
^C[2018-03-07T16:05:02,473][WARN ][logstash.runner          ] SIGINT received. Shutting down.
[2018-03-07T16:05:03,446][INFO ][logstash.pipeline        ] Pipeline has terminated {:pipeline_id=>"main", :thread=>"#<Thread:0x73d5787a run>"}


コンフィグをrubydebugにしておかないと、field表示ない
これめちょっくわかりにくくね？ってかどんなときにjsonするのか謎


_failureみたいなはなし
//cmd{
{
            "tags" => [
        [0] "git-log"
    ],
    "author_email" => "froakie002@gmail.com",
     "author_name" => "MofuMofu2",
         "subject" => "[fix] gitコンフィグでちゃんとしたjsonが出力できるように調整",
            "host" => "ishiiaoi-no-MacBook-Pro.local",
         "message" => " {\"commit_hash\":\"cc97e0a\",\"author_name\":\"MofuMofu2\",\"author_email\":\"froakie002@gmail.com\",\"author_date\":\"2018-03-07T13:11:30+09:00\",\"subject\":\"[fix] gitコンフィグでちゃんとしたjsonが出力できるように調整\"}",
     "author_date" => "2018-03-07T13:11:30+09:00",
        "@version" => "1",
     "commit_hash" => "cc97e0a",
      "@timestamp" => 2018-03-07T07:36:51.297Z,
            "path" => "/Users/mallow/log/gitLog.json"
}
//}

//cmd{
{
      "@version" => "1",
          "tags" => [
        [0] "git-log",
        [1] "_jsonparsefailure"
    ],
    "@timestamp" => 2018-03-07T07:36:51.297Z,
          "path" => "/Users/mallow/log/gitLog.json",
          "host" => "ishiiaoi-no-MacBook-Pro.local",
       "message" => " {\"commit_hash\":\"1a397cb\",\"author_name\":\"MofuMofu2\",\"author_email\":\"froakie002@gmail.com\",\"author_date\":\"2018-03-07T14:08:27+09:00\",\"subject\":\"[fix] 解消が間違ってたので修正\"},"
}
//}
