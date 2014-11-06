#!/bin/bash

ROOT_DIR="$(dirname $(realpath $(dirname "${BASH_SOURCE[0]}")))"
THIRD_PARTY_DIR="$ROOT_DIR/third_party/"

function install_dep_from_tarfile {
  SRC_URL=$1
  PACKAGE_DIR=$2
  FILENAME="$(basename $SRC_URL)"

  DOWNLOAD_DIR="$(basename $FILENAME .tar.gz)/$PACKAGE_DIR"
  INSTALL_DIR="$THIRD_PARTY_DIR/$PACKAGE_DIR"
  OLD_DIR="$THIRD_PARTY_DIR/$PACKAGE_DIR.old"

  mkdir -p "$INSTALL_DIR"
  cd "$INSTALL_DIR"
  echo "Downloading $SRC_URL"
  curl --remote-name "$SRC_URL"
  tar xvzf "$FILENAME"

  cd "$DOWNLOAD_DIR"

  # Replace with new directory
  cd "$ROOT_DIR"
  mv "$INSTALL_DIR" "$OLD_DIR"
  mv "$OLD_DIR/$DOWNLOAD_DIR" "$INSTALL_DIR"
  rm -fr "$OLD_DIR"
}

# BODY

# Download and extract PLY.
# Homepage:
# http://dabeaz.com/ply
install_dep_from_tarfile "http://dabeaz.com/ply/ply-3.4.tar.gz" "ply"

# Download and extract Jinja2.
# Homepage:
# http://jinja.pocoo.org/
# Installation instructions:
# http://jinja.pocoo.org/docs/intro/#from-the-tarball-release
# Download page:
# https://pypi.python.org/pypi/Jinja2
JINJA2_SRC_URL="https://pypi.python.org/packages/source/"
JINJA2_SRC_URL+="J/Jinja2/Jinja2-2.7.1.tar.gz"
install_dep_from_tarfile $JINJA2_SRC_URL 'jinja2'

# Download and extract MarkupSafe.
# Homepage:
# https://pypi.python.org/pypi/MarkupSafe
MARKUPSAFE_SRC_URL="https://pypi.python.org/packages/source/"
MARKUPSAFE_SRC_URL+="M/MarkupSafe/MarkupSafe-0.23.tar.gz"
install_dep_from_tarfile $MARKUPSAFE_SRC_URL 'markupsafe'

# Download and extract Cython.
mkdir -p $THIRD_PARTY_DIR/cython
cd $THIRD_PARTY_DIR/cython
curl --remote-name http://cython.org/release/Cython-0.20.2.zip
unzip Cython-0.20.2.zip
rm -rf Cython-0.20.2.zip
mv Cython-0.20.2 src
