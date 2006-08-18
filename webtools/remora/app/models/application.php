<?php

class Application extends AppModel
{
    var $name = 'Application';
    var $hasOne = array('Appversion' =>
                         array('className'   => 'Appversion',
                               'conditions'  => '',
                               'order'       => '',
                               'limit'       => '',
                               'foreignKey'  => 'application_id',
                               'dependent'   => true,
                               'exclusive'   => false,
                               'finderSql'   => ''
                         ),
                         'Tag' =>
                         array('className'   => 'Tag',
                               'conditions'  => '',
                               'order'       => '',
                               'limit'       => '',
                               'foreignKey'  => 'application_id',
                               'dependent'   => false,
                               'exclusive'   => false,
                               'finderSql'   => ''
                         ),

                  );
    var $hasAndBelongsToMany = array('Version' =>
                                      array('className'  => 'Version',
                                            'joinTable'  => 'applications_versions',
                                            'foreignKey' => 'application_id',
                                            'associationForeignKey'=> 'version_id',
                                            'conditions' => '',
                                            'order'      => '',
                                            'limit'      => '',
                                            'unique'     => false,
                                            'finderSql'  => '',
                                            'deleteQuery'=> '',
                                      ),
                                      );
}
?>
