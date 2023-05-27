#!/bin/bash

FOLDER="cpp-backend-tests-practicum"

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
REPO=$(real_dir "$(dirname "$0")")

cd ${REPO} || exit 1

if ! git clone git@github.com:cpppracticum/"$FOLDER".git "$FOLDER"; then
  (
  cd "$FOLDER" || exit 1
  git pull
  )
fi


if [ ! -d "${REPO}/.venv" ] ; then
  python3 -m venv ${REPO}/.venv
fi

source ${REPO}/.venv/bin/activate
python3 -m pip install --upgrade pip
python3 -m pip install -r requirements.txt