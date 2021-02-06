#!/usr/bin/env bash

echo "Installing GPG keys for git..."

if [ -z "${GPG_K}" ] || [ -z "${GPG_K}" ] || [ -z "${GPG_KEY}" ]; then
    echo "GPG_K and GPG_I must be defined" >&2
    exit 1
fi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

openssl aes-256-cbc -K $GPG_K -iv $GPG_I -in $DIR/gpg-keyring.gpg.enc -out ~/gpg-keyring.gpg -d
gpg --batch --import ~/gpg-keyring.gpg
rm -f ~/gpg-keyring.gpg
git config --global user.name 'Aliaksei Stratsilatau'
git config --global user.email 'sa@uavos.com'
git config --global user.signingkey $GPG_KEY

echo "GPG keys installed."
