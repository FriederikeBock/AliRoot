#!/bin/bash

if [ $1 == 2010 ]; then
    root -x -q -l 'simple_geom_generate.C("TRD",197584,2010)'
    root -x -q -l 'simple_geom_generate.C("EMC",197584,2010)'
elif [ $1 == 2011 ]; then
    root -x -q -l 'simple_geom_generate.C("TRD",197584,2011)'
    root -x -q -l 'simple_geom_generate.C("EMC",197584,2011)'
elif [ $1 == 2012 ]; then
    root -x -q -l 'simple_geom_generate.C("TRD",197584,2012)'
    root -x -q -l 'simple_geom_generate.C("EMC",197584,2011)'
fi
    