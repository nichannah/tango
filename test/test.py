
from __future__ import print_function

import os
import tango as coupler

class TestInterface:

    def __init__(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))

    def test_init(self):

        config = os.path.join(self.test_dir, 'config.yaml')
        tango = coupler.Tango(config, 'ocean', 0, 1, 0, 1, 0, 1, 0, 1)
