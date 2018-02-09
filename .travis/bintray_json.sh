#!/bin/bash

# Set the distribution name
DIST=xenial,yakkety,zesty,artful,bionic

#$1 = Repository
#$2 = Version
#$3 = Output filename
print_json () {
DATE=`date --iso-8601`

cat > ${3} <<EOF
{
    "package": {
        "name": "souffle",
        "repo": "$1",
        "subject": "souffle-lang",
        "website_url": "http://souffle-lang.org/",
        "issue_tracker_url": "https://github.com/souffle-lang/souffle/issues",
        "vcs_url": "https://github.com/souffle-lang/souffle.git",
        "github_use_tag_release_notes": true,
        "github_release_notes_file": "debian/changelog",
        "licenses": ["UPL"],
        "public_download_numbers": false,
        "public_stats": false
    },

    "version": {
        "name": "$2",
        "released": "$DATE"
    },

    "files":
    [{"includePattern": "deploy/(.*\.deb)", "uploadPattern": "pool/main/s/souffle/\$1",
    "matrixParams": {
        "deb_distribution": "$DIST",
        "deb_component": "main",
        "deb_architecture": "amd64",
        "override": 1 }
    }],
    "publish": true
}
EOF
}

print_json "deb"  "`git describe --tags --always`" "bintray-stable.json"
print_json "deb-unstable" "`git describe --tags --always`" "bintray-unstable.json"
