fbsdpt3
=======
FreeBSD用のPT3デバイスドライバ

http://code.google.com/p/ptx-kmod/

https://github.com/m-tsudo/pt3


この辺を参考にしながら。

地デジチューナーが動作しました。BSは不明。

予期せず固まりますので要注意状態。（ドライバのアンロードなど）


以下オリジナル版のREADME


==================================================
earthsoft PT3 driver
==================================================

ライセンス
GPLv3です。ライセンス内容はCOPYINGをご確認ください。

パラメータ
lnb   LNBのデフォルト値を指定します
      0: OFF (default)
      1: 11V
      2: 15V
debug メッセージの表示を制御します
      0: 出来るだけ表示しない (default)
      1: pt1と同じ程度表示します
      7: デバッグ用メッセージも表示します
      ※/sys/module/pt3_drv/parameters/debugからも変更できます
      例： echo 7 > /sys/module/pt3_drv/parameters/debug


ptx - FreeBSD driver for PT1/PT2
--------------------------------
						http://d.hatena.ne.jp/bsdaemon/


FreeBSDの、EARTHSOFT PT1/PT2デバイスドライバです。

Linuxのchardev版ドライバがベースです。
http://hg.honeyplanet.jp/pt1/

**** お約束 ****
このドライバにより生じるかもしれない、いかなる損害についても
無保障です。
panicしたり固まったりしても、慌てず騒がずリブートできる人向け。

開発環境は下の通り。

 FreeBSD/amd64 8.0-RELEASE
 athlon64X2 6000+
 PT2 rev.A x 2

 FreeBSD/amd64 9.0-STABLE
 atomD525
 PT2 rev.A


INSTALL
-------

 # cd dev/ptx
 # make
 # make install


SETUP
-----

 # kldload ptx.ko

 正常に読み込まれると、/dev/ptxN.{s0,s1,t0,t1} が作成されます。

  ptxN.s[01] - ISDB-Sデバイス
  ptxN.t[01] - ISDB-Tデバイス

 また、sysctl MIB dev.ptx.N が作成されます。

  dev.ptx.N.lnb [RW]
	val = { OFF:0 +11V:1 +15V:2 }

  dev.ptx.N.s0.freq	BSデジタルチューナ0のチューニング[W]
	val = freqno + (slot << 16)
	具体的には tools/recptx.pl を参照

  dev.ptx.N.s0.signal	BSデジタルチューナ0の信号強度[R]
  	val = (詳細不明)

  dev.ptx.N.t0.freq	地上デジタルチューナ0のチューニング[W]
  dev.ptx.N.t0.signal	地上デジタルチューナ0の信号強度[R]
  dev.ptx.N.s1.freq	BSデジタルチューナ1のチューニング[W]
  dev.ptx.N.s1.signal	BSデジタルチューナ1の信号強度[R]
  dev.ptx.N.t1.freq	地上デジタルチューナ1のチューニング[W]
  dev.ptx.N.t1.signal	地上デジタルチューナ1の信号強度[R]


USAGE
-----

  ex. 地上波デジタルNTVにチューニングして、TSを保存

  sysctl dev.ptx.0.t0.freq=75
  cat /dev/ptx0.t0 > ntv.ts


KNOWN BUGS
----------

・PT1/2を複数枚挿して録画すると、micropacket errorが発生し、そのうちpage faultで
  panicする(というレポートをもらっているが、開発環境で再現できていない)


HISTORY
-------

2010-04-30
    公開

2010-10-04
    up0283パッチ(boot時に読み込ませると初期化に失敗する問題への対処)を適用。

    録画開始、チャンネル変更、信号強度取得など、デバイスへのコマンドを同時、
    もしくは短い間隔で発行するとpanicする問題を修正。

2011-02-06
    tools/recptx.plを２つ同時に起動すると、片方がdevice busyになる問題に
    やや対処。完全に同時だとだめかも。

2011-03-26
    PT1/2を複数枚挿して録画するとpage faultでpanicする問題に対処したつもり。
    (開発環境では問題再現できなかったので、あくまでつもり)

    up0308パッチ(変数の型を整理)を適用。

2011-11-12
    multimedia/ptx-kmodのパッチ(FreeBSD-8より前でbuild可、他)を適用。

    2senのパッチを適用。
      up0310パッチ(kldunload不具合対応、他) by よっちいさま
      up0345パッチ(tools/recptx.plの新BS対応) by yamajunさま

2012-05-19
    BS-17のtuner dataの間違い修正 by aさま

    GoogleCodeにレポジトリ作成


