<?php

$mbox = imap_open("{imap.example.org}INBOX", "username", "password")
      or die("Connection failed: " . imap_last_error());

$check = imap_mailboxmsginfo($mbox);

if ($check)
    echo "[" . $check->Unread . "]";
else
    echo "imap_mailboxmsginfo() failed: " . imap_last_error() . "<br />\n";

imap_close($mbox);

?>