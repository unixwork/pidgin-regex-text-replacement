# pidgin-regex-text-replacement

This plugin for the [Pidgin][1] instant messenger allows to define regex based text replacement rules for outgoing messages.

# Build

Run `make` to build the plugin.

Run `make install` to install the plugin to `~/.purple/plugins`. Once installed, the *Plugins* list in Pidgin should contain an entry *Regex Text Replacement*.

# Usage

Configuration can be done via Pidgin Plugin GUI, but it is also possible to directly edit the file `~/.purple/regex-text-replacement.rules`.

Each rule consists of:
 - A regex pattern
 - A replacement text
 
Capture groups in the regex pattern can be referenced in the replacement text using `$1` (only one capture group is supported).

The replacement also supports standard escape sequences like `\t` or `\n`. `$1` can be escaped with `\$1`.

# File Format

The `~/.purple/regex-text-replacement.rules` file must start with `?v1`. Each subsequent line defines a replacement rule. Rules are structured as follows:

    pattern <TAB> replacement

Example:

    ?v1
    gh#([0-9]+)	<a href="https://github.com/unixwork/pidgin-regex-text-replacement/issues/$1">issue #$1</a>

This rule would replace gh#123 with a link to the corresponding GitHub issue.


[1]: https://pidgin.im/
