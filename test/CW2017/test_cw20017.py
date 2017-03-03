
import os
import sys
import sh
import subprocess as sp
import pytest

data_tarball_url = 'http://s3-ap-southeast-2.amazonaws.com/dp-drop/esmgrids/test/test_data.tar.gz'

class TestCw2017():

    @pytest.fixture
    def input_dir(self):
        """
        Download input data if it doesn't already exist.
        """
        my_dir = os.path.dirname(os.path.realpath(__file__))
        test_dir = os.path.join(my_dir, '../')
        test_data_dir = os.path.join(test_dir, 'test_data')
        test_data_tarball = os.path.join(test_dir, 'test_data.tar.gz')
        if not os.path.exists(test_data_dir):
            if not os.path.exists(test_data_tarball):
                sh.wget('-P', test_dir, data_tarball_url)
            sh.tar('zxvf', test_data_tarball, '-C', test_dir)

        return os.path.join(test_data_dir, 'input')

    def test_build(self):
        """
        Run make to build Tango and OASIS tests.
        """
        pass

    def test_build_remapping_weights(self, input_dir):
        """
        Make remapping weights files.
        """

        my_dir = os.path.dirname(os.path.realpath(__file__))

        cmd = [os.path.join(my_dir, 'oasis-grids', 'remapweights.py')]

        weights = 'atm_to_ice_rmp.nc'
        core2_hgrid = os.path.join(input_dir, 't_10.0001.nc')
        ice_hgrid = os.path.join(input_dir, 'ocean_hgrid.nc')
        ice_mask = os.path.join(input_dir, 'ocean_mask.nc')
        args = ['CORE2', 'MOM', '--src_grid', core2_hgrid,
                '--dest_grid', ice_hgrid, '--dest_mask', ice_mask,
                '--method', 'conserve', '--output', weights]
        ret = sp.call(cmd + args)
        assert ret == 0
        assert os.path.exists(weights)

    def test_run_small(self):
        """
        Run in 5 processes.
        """
        pass

    def test_run_big(self):
        """
        Run on ~3000 processes.
        """
        pass
