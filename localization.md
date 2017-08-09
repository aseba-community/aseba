# Localization guidelines

Several Aseba tools are translated into several languages.
For now, Aseba Studio, Aseba Challenge, and the error messages from the Aseba compiler are available in French, German, Spanish, Italian, Greek, Chinese.
The translations for Japanese are in progress…

## Help us

We are always looking for motivated translators.
The amount of work is currently around 500 translations for the whole project, mainly single words and short sentences.
The reference language is English.

Here are the steps to help us:

### Your language is German, Spanish, Greek, French, Italian, Japanese or Chinese

In that case, the translation files already exist.
To modify the translation, first [fork the Aseba repository](https://github.com/aseba-community/aseba#fork-destination-box) ([understand github forking](https://help.github.com/articles/fork-a-repo/)).
Then, modify the relevant files, these are:
* For Studio: `clients/studio/asebastudio_LL.ts` and `clients/studio/compiler_LL.ts`
* For Thymio Firmware Updater: `clients/thymioupgrader/thymioupgrader_LL.ts`
* For Thymio Wireless Configurator: `clients/thymiownetconfig/thymiownetconfig_LL.ts`
* For Aseba Challenge: `targets/challenge/asebachallenge_LL.ts`
Where `LL` is the [ISO 639-1](https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes) code of your language.

Then, submit a [pull request](https://help.github.com/articles/about-pull-requests/) with the updated files.

### Your language is a new one

Please open an [new issue](https://github.com/aseba-community/aseba/issues/new) requesting the addition of your language.
Once the issue is processed, follow the instructions above.

### Useful Resources

Qt translation tools (Qt Linguist):

* [Main user manual](http://qt-project.org/doc/qt-4.8/linguist-manual.html)
* [For translators](http://qt-project.org/doc/qt-4.8/linguist-translators.html)
* [For release managers](http://qt-project.org/doc/qt-4.8/linguist-manager.html)

Databases with existing translations:

* [KDE Localization search engine](http://i18n.kde.org/dictionary/search-translations.php)

### For Maintainers

In [`/maintainer/translations`](https://github.com/aseba-community/aseba/tree/master/maintainer/translations) you will find a set of scripts to help you maintain the translation files.
