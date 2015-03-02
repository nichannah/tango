
from __future__ import print_function

import sys
import unittest
import os
import tango as coupler

class TestInterface(unittest.TestCase):

    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))

    def test_init(self):

        config = os.path.join(self.test_dir, 'config.yaml')
        print('Config = {}'.format(config))
        tango = coupler.Tango()
        id = tango.init(config, 'ocean', 0, 1, 0, 1, 0, 1, 0, 1)
        print('Id is {}'.format(id))

    def test_other(self):
        pass

if __name__ == '__main__':
    # This interpreter doesn't like exit() being called, then there is no
    # chance to call MPI_Finalize()
    try:
        unittest.main()
    except SystemExit:
        pass
