<?php
/**
 * This will delete a session if the user has one.
 *
 * @package amo
 * @subpackage docs
 *
 */

startProcessing('logout.tpl', null, null, 'rustico');
require_once 'includes.php';

session_start();

$_auth->removeUsernameCookie();

session_destroy();

// Assign template variables.
$tpl->assign(
    array(  'title'             => 'Firefox Add-ons',
            'currentTab'        => null
            )
);
?>
