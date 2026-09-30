#!/bin/sh
# Stages the microG APKs that Android.mk installs. Git carries their checksums
# in microg.sha256; the APKs ship as the zstd tarball asset of this
# repository's release microg-v0.3.17.252432, because GmsCore exceeds GitHub's
# 100 MB file limit. An APK set that already matches microg.sha256 is kept.
#
# Usage: sh prebuilt/microg/fetch.sh
set -eu

dir=$(cd "$(dirname "$0")" && pwd)
tag=microg-v0.3.17.252432
asset=$tag.tar.zst
asset_sha256=76599dbc44763fd43eb360d07af9bce2a9c6e1fd3c976eaf6dc11eb64408fc47
url=https://github.com/Oichkatzelesfrettschen/android_device_htc_a11/releases/download/$tag/$asset

cd "$dir"
if sha256sum --check --status microg.sha256 2>/dev/null; then
	exit 0
fi

tmp=$(mktemp -d)
trap 'rm -f "$tmp"/*; rmdir "$tmp"' EXIT
curl -fsSL -o "$tmp/$asset" "$url"
echo "$asset_sha256  $tmp/$asset" | sha256sum --check --quiet
zstd -dcq "$tmp/$asset" | tar -xf - -C "$tmp"
(cd "$tmp" && sha256sum --check --quiet "$dir/microg.sha256")
while read -r _ apk; do
	cp "$tmp/$apk" "$dir/$apk"
done < microg.sha256
