#!/bin/bash


#$1 = Repository
#$2 = Version
#$3 = Output filename
#$4 = Repository type (deb/rpm)
print_json () {
DATE=`date --iso-8601`

if [ "$4" = rpm ];
then
  #For distro dependent settings
  RELEASE=`grep "^VERSION_ID=" /etc/os-release|sed 's/VERSION_ID=//'|tr -d '"'`
  DIST=`grep "^ID=" /etc/os-release|sed 's/ID=//'|tr -d '"'`
  ARCH=$(uname -i)

  #Fedora 27 x86_64
  #DIST='fedora'
  #RELEASE='27'
  #ARCH='x86_64'

  FILES="[{\"includePattern\": \"deploy/(.*\.rpm)\", \"uploadPattern\": \"$DIST/$RELEASE/$ARCH/\$1\"
    }],"
else
  DIST=xenial,yakkety,zesty,artful,bionic

  FILES="[{\"includePattern\": \"deploy/(.*\.deb)\", \"uploadPattern\": \"pool/main/s/souffle/\$1\",
    \"matrixParams\": {
        \"deb_distribution\": \"$DIST\",
        \"deb_component\": \"main\",
        \"deb_architecture\": \"amd64\",
        \"override\": 1 }
    }],"
fi

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
    $FILES
    "publish": true
}
EOF
}

print_json "deb"  "`git describe --tags --always`" "bintray-deb-stable.json"
print_json "deb-unstable" "`git describe --tags --always`" "bintray-deb-unstable.json"
print_json "rpm"  "`git describe --tags --always`" "bintray-rpm-stable.json" "rpm"
print_json "rpm-unstable"  "`git describe --tags --always`" "bintray-rpm-unstable.json" "rpm"
