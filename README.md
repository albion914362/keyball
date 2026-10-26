# albion の Keyball61

![Keyball61](./keyball61/doc/rev1/images/kb61_001.jpg)

[Yowkees/keyball](https://github.com/Yowkees/keyball) をフォークして、自分用の Keyball61 の設定だけを残したリポジトリです。
ほかの機種（Keyball39 / 44 / 46 / One47）と Keybit8 は削除しています。必要なときは公式リポジトリを見てください。

## 中身

| 場所 | 内容 |
|------|------|
| [tool/keyball61設定変更ツール.html](./tool/keyball61設定変更ツール.html) | 設定変更ツール。ブラウザで開いて config.h / keymap.c / rules.mk を作り、GitHub でビルド、.hex の書き込みまでできる |
| [qmk_firmware/keyboards/keyball/keyball61/keymaps/albion](./qmk_firmware/keyboards/keyball/keyball61/keymaps/albion) | 自分のキーマップ |
| [qmk_firmware/keyboards/keyball](./qmk_firmware/keyboards/keyball) | Keyball61 のファームウェアのソース（共通部品の lib/ と drivers/ を含む） |
| [keyball61/doc](./keyball61/doc/rev1) | 組み立て方（[日本語](./keyball61/doc/rev1/buildguide_jp.md) / [English](./keyball61/doc/rev1/buildguide_en.md)） |
| [keyball61/design_data](./keyball61/design_data) | 基板・アクリルプレートのデータ |

## ファームウェアのビルド

GitHub の Actions で作ります。

* 設定変更ツールの「GitHub でビルド」から実行する（ふだんはこちら）
* または Actions の「Build a firmware on demand」を手動で実行し、keymap に `albion` を指定する

できた .hex は Actions の実行結果の Artifacts からダウンロードし、ツールの「書き込み (.hex)」で左右それぞれに書き込みます。

## ライセンス

元のリポジトリと同じ [GPL-3.0](./LICENSE) です。キーボードの設計とファームウェアは [@Yowkees](https://github.com/Yowkees) さんによるものです。
