# Common Source Code Project

ここは、ぽっぽ／目玉Pさんが作成してくれた「Common Source Code Project」を自分用にカスタマイズする場所です。


## 現在対応したこと
* （全体）
  - ゲーム選択時に、ルートからじゃなくサブディレクトリから選べるようにした
  - 未接続ジョイスティック暴走の防止＆外部パッド対応
  - 全機種のソフトキーボード反応改善（KEY_KEEP_FRAMES = 8）
  - GitHub Actions の全機種並列・64bit特化による爆速ビルド化（時間かかるので😅）
  - フォルダ記憶＆[UP] 時の即座位置復元
* (MSX)
  - MSXの16KB ROM（TEXT ポインタ起動）の対応
 
## Screenshots

<table>
  <tr>
    <td width="50%" align="center"><img src="screenshots/dark-mode-file-menu.png" alt="Select" valign="bottom"></td>
    <td width="50%" align="center"><img src="screenshots/donkey-kong-country-retroachievements.gif" alt="Donkey Kong Country gameplay" valign="bottom"></td>
  </tr>
  <tr>
    <td valign="top" width="50%" align="center">日本語ファイル名表示, Title enabled</td>
    <td valign="top" width="50%" align="center">Pause screen, per-game overlay enabled</td>
  </tr>
  <tr><td colspan="2"></td></tr>
  <tr></tr>
  <tr>
    <td width="50%" align="center"><img src="screenshots/dark-mode-file-menu.png" alt="Select" valign="bottom"></td>
    <td width="50%" align="center"><img src="screenshots/donkey-kong-country-retroachievements.gif" alt="Donkey Kong Country gameplay" valign="bottom"></td>
  </tr>
  <tr>
    <td valign="top" width="50%" align="center">日本語ファイル名表示, Title enabled</td>
    <td valign="top" width="50%" align="center">Pause screen, per-game overlay enabled</td>
  </tr>
  <tr><td colspan="2"></td></tr>
  <tr></tr>
</table>




## Credits

* The original Android port was started by [Nochi](https://github.com/shikarunochi)
  (Twitter [@shikarunochi](https://x.com/shikarunochi)).
* medamap, [Common Source Code Project](https://github.com/medamap/CommonSourceCodeProject), for creating the excellent base this fork builds on

## Notes

- Upstream license notice: [License_notice.md](License_notice.md)
- Project documentation: [docs/README.md](docs/README.md)
- Overview: [docs/overview.md](docs/overview.md)
- Build process: [docs/build-process.md](docs/build-process.md)
- Project wiki: <https://github.com/medamap/CommonSourceCodeProject/wiki>
- GPLv2 text: [`license/COPYING.txt`](license/COPYING.txt)
