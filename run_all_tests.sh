#!/bin/bash

WORKDIR=/home/addaswsw/lab/tiger-compiler-26sp
main_script=${WORKDIR}/scripts/lab5_test/main.py
testcase_dir=${WORKDIR}/testdata/lab5or6/testcases
ref_dir=${WORKDIR}/testdata/lab5or6/refs
mergecase_dir=$testcase_dir/merge
mergeref_dir=$ref_dir/merge
score=0
full_score=1

echo "========== Lab5 Test =========="

for testcase in "$testcase_dir"/*.tig; do
  testcase_name=$(basename "$testcase" | cut -f1 -d".")
  ref=${ref_dir}/${testcase_name}.out
  assem=$testcase.s

  # Generate assembly if not exists or source is newer
  if [[ ! -f "$assem" ]] || [[ "$testcase" -nt "$assem" ]]; then
    ${WORKDIR}/build/test_codegen "$testcase" >&/dev/null
  fi

  if [[ $testcase_name == "merge" ]]; then
    for mergecase in "$mergecase_dir"/*.in; do
      mergecase_name=$(basename "$mergecase" | cut -f1 -d".")
      mergeref=${mergeref_dir}/${mergecase_name}.out
      python3 ${main_script} ${assem} <"$mergecase" >&/tmp/output.txt
      diff -w -B /tmp/output.txt "$mergeref" >&/dev/null
      if [[ $? != 0 ]]; then
        echo "Error: Output mismatch [$testcase_name/$mergecase_name]"
        full_score=0
        continue
      fi
      score=$((score + 5))
      echo "Pass $testcase_name/$mergecase_name"
    done
  else
    python3 ${main_script} ${assem} >&/tmp/output.txt
    diff -w -B /tmp/output.txt "$ref" >&/dev/null
    if [[ $? != 0 ]]; then
      echo "Error: Output mismatch [$testcase_name]"
      echo "Expected:"
      cat "$ref"
      echo "Got:"
      cat /tmp/output.txt
      full_score=0
      continue
    fi
    echo "Pass $testcase_name"
    score=$((score + 5))
  fi
done

if [[ $full_score == 0 ]]; then
  echo "LAB5 part2 SCORE: ${score}"
else
  echo "[^_^]: Pass"
  echo "LAB5 part2 SCORE: 100"
fi
