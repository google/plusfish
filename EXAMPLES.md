
There are some basic examples for running scans. For a full list of flags, just
run ./plusfish --helpfull.

## Run all security checks

The checks_config_path flags determines what security checks are loaded. In this
example, all tests are loaded and thus will be performed.

    ./plusfish \
     --checks_config_path="config/*.asciipb" \
     --report_type=JSON --json_report_file=/tmp/report.json \
     http://url1 http://url2 ...

## Run only one specific security check

This would only run the SQL injection checks:

    ./plusfish \
     --checks_config_path="config/sql-injection.asciipb" \
     --report_type=JSON --json_report_file=/tmp/report.json \
     http://url1 http://url2 ...

## Crawl only mode

Loading no security checks will cause plusfish to only crawl a site.

    ./plusfish \
     --report_type=JSON --json_report_file=/tmp/report.json \
     http://url1 http://url2 ...

## Test URLs from a file and disable crawling

This can be useful when you only want to test a specific list of URLs and don't
want plusfish to touch other pages.

    ./plusfish \
     --noextract_links \
     --checks_config_path="config/*.asciipb" \
     --report_type=JSON --json_report_file=/tmp/report.json \
     --url_file <<file with urls>>

## Sending custom headers to a single domain

Sometimes you want send a special (magic) header to a single domain and this can
be done using the --default_headers flags.

    ./plusfish \
     --checks_config_path="config/*.asciipb" \
     --default_headers=domain.needs.header.com:X-Header-Name=Value \
     --report_type=JSON --json_report_file=/tmp/report.json \
     --url_file <<file with urls>>

If instead you want to send a special header to _all_ domains:

    ./plusfish \
     --checks_config_path="config/*.asciipb" \
     --default_headers=*:X-Header-Name=Value \
     --report_type=JSON --json_report_file=/tmp/report.json \
     --url_file <<file with urls>>

