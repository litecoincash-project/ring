#!/usr/bin/env bash

export LC_ALL=C
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

RINGD=${RINGD:-$BINDIR/ringd}
RINGCLI=${RINGCLI:-$BINDIR/ring-cli}
RINGTX=${RINGTX:-$BINDIR/ring-tx}
WALLET_TOOL=${WALLET_TOOL:-$BINDIR/ring-wallet}
RINGQT=${RINGQT:-$BINDIR/qt/ring-qt}

[ ! -x $RINGD ] && echo "$RINGD not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
RNGVER=($($RINGCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for ringd if --version-string is not set,
# but has different outcomes for ring-qt and ring-cli.
echo "[COPYRIGHT]" > footer.h2m
$RINGD --version | sed -n '1!p' >> footer.h2m

for cmd in $RINGD $RINGCLI $RINGTX $WALLET_TOOL $RINGQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${RNGVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${RNGVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
