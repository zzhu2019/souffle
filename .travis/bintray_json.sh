#!/bin/bash

DATE=`date --iso-8601`

cat > bintray-latest.json <<EOF
{
    "package": {
        "name": "souffle",
        "repo": "deb",
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
        "name": "Development",
        "desc": "HEAD from development branch",
        "released": "$DATE",
        "gpgSign": false
    },

    "files":
    [{"includePattern": "deploy/(.*\.deb)", "uploadPattern": "pool/main/s/souffle/\$1",
    "matrixParams": {
        "deb_distribution": "xenial",
        "deb_component": "main",
        "deb_architecture": "amd64",
        "override": 1 }
    }],
    "publish": true
}
EOF

