#!/bin/bash
opt -dot-cfg -dot-dom -dot-postdom $1

dot -Tpdf dom.nest1.dot > dom.pdf
dot -Tpdf postdom.nest1.dot > postdom.pdf
dot -Tpdf cfg.nest1.dot > cfg.pdf
