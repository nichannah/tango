
import os
import sys
import xarray as xr
import pandas as pd
import numpy as np
from mpi4py import MPI
import yaml
sys.path.append('../../lib')
import tango as coupler

class CoreNyfData:
    forcings = {'Q_10' : 'q_10.0001.nc', 'U_10' : 'u_10.0001.nc',
                'V_10' : 'v_10.0001.nc', 'T_10' : 't_10.0001.nc'}
    a_field = 'T_10'

    def __init__(self):
        my_dir = os.path.dirname(os.path.realpath(__file__))
        test_data_dir = os.path.join(my_dir, '../', 'test_data', 'input')

        for k, v in self.forcings.items():
            self.forcings[k] = os.path.join(test_data_dir, v)

    def get_res(self):
        with xr.open_dataset(self.forcings[self.a_field]) as d:
            return d[self.a_field].shape[1:]

    def get_timestamps(self):
        with xr.open_dataset(self.forcings[self.a_field]) as d:
            return map(lambda x : x.to_datetime64(),
                       pd.to_datetime(d['TIME'].data))

    def lookup_data(self, varname, timestamp):
        varname = varname.upper()
        with xr.open_dataset(self.forcings[varname]) as d:
            return np.array(d.sel(TIME=timestamp)[varname].data,
                            dtype=np.float64)

def get_coupling_fields(config):
    with open(config) as f:
        conf = yaml.load(f)
        return conf['mappings'][0]['fields']


def main():

    core = CoreNyfData()
    rows, cols = core.get_res()
    fieldnames = get_coupling_fields('./config.yaml')

    tango = coupler.Tango('./', 'atm', 0, cols, 0, rows, 0, cols, 0, rows)

    # Read in atmosphere coupling fields and send
    for timestamp in core.get_timestamps():
        tango.begin_transfer(str(timestamp), 'ice')

        for fname in fieldnames:
            tango.put(fname, core.lookup_data(fname, timestamp))

        tango.end_transfer()

    tango.finalize()

if __name__ == '__main__':
    sys.exit(main())
