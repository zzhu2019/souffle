#!/bin/bash

DATE=`date --iso-8601`
if [ -z $1 ]
then
  echo "No distribution name was given"
  exit 1
else
  DIST=$1
fi
if [ -z $2 ]
then
  REPO=deb
  VERSION=`git describe --abbrev=0 --tags`
else
  REPO=deb-unstable
  VERSION="`git describe --abbrev=0 --tags`-`git rev-parse --short HEAD`" 
fi

cat > bintray.json <<EOF
{
    "package": {
        "name": "souffle",
        "repo": "$REPO",
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
        "name": "$VERSION",
        "desc": "HEAD from development branch",
        "released": "$DATE",
        "gpgSign": false
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

