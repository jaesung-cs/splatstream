""" This script is used to download the DL3DV benchmark from the huggingface repo.

    The benchmark is composed of 140 different scenes covering different scene complexities (reflection, transparency, indoor/outdoor, etc.) 

    The whole benchmark is very large: 2.1 TB. So we provide this script to download the subset of the dataset based on common needs. 


        - [x] Full benchmark downloading
            Full download can directly be done by git clone (w. lfs installed).

        - [x] scene downloading based on scene hash code  

        Option: 
        - [x] images_4 (960 x 540 resolution) level dataset (approx 50G)

"""

import os 
from os.path import join
import pandas as pd
from tqdm import tqdm
from huggingface_hub import HfApi 
import argparse
import traceback
import pickle
import shutil
import json
import numpy as np
import cv2

api = HfApi()
repo_root = 'DL3DV/DL3DV-10K-Benchmark'


def hf_download_path(repo_path: str, odir: str, max_try: int = 5):
    """ hf api is not reliable, retry when failed with max tries

    :param repo_path: The path of the repo to download
    :param odir: output path 
    """	
    rel_path = os.path.relpath(repo_path, repo_root)
    rel_path = rel_path.replace("\\", "/")

    counter = 0
    while True:
        if counter >= max_try:
            print("ERROR: Download {} failed.".format(repo_path))
            return False

        try:
            api.hf_hub_download(repo_id=repo_root, filename=rel_path, repo_type='dataset', local_dir=odir, cache_dir=join(odir, '.cache'))
            return True

        except BaseException as e:
            traceback.print_exc()
            counter += 1
            print(f'Retry {counter}')
            print(rel_path)
            print(repo_root)
    

def clean_huggingface_cache(cache_dir: str):
    """ Huggingface cache may take too much space, we clean the cache to save space if necessary

    :param cache_dir: the current cache directory 
    """    
    # Current huggingface hub does not provide good practice to clean the space.  
    # We mannually clean the cache directory if necessary. 
    shutil.rmtree(join(cache_dir, 'datasets--DL3DV--DL3DV-10K-Benchmark'))


def download_by_hash(filepath_dict: dict, odir: str, hash: str, only_level4: bool):
    """ Given a hash, download the relevant data from the huggingface repo 

    :param filepath_dict: the cache dict that stores all the file relative paths 
    :param odir: the download directory 
    :param hash: the hash code for the scene 
    :param only_level4: the images_4 resolution level, if true, only the images_4 resolution level will be downloaded 
    """	
    all_files = filepath_dict[hash]
    download_files = [join(repo_root, f) for f in all_files] 

    if only_level4: # only download images_4 level data
        download_files = []
        for f in all_files:
            subdirname = os.path.basename(os.path.dirname(f))

            if 'images' in f and subdirname != 'images_4' or 'input' in f:
                continue 

            download_files.append(join(repo_root, f))

    for f in download_files:
        if hf_download_path(f, odir) == False:
            return False

    return True
    

def download_benchmark(args):
    """ Download the benchmark based on the user inputs.

        1. download the benchmark-meta.csv
        2. based on the args, download the specific subset 
            a. full benchmark 
            b. full benchmark in images_4 resolution level 
            c. full benchmark only with nerfstudio colmaps (w.o. gaussian splatting colmaps) 
            d. specific scene based on the index in [0, 140)

    :param args: argparse args. Used to decide the subset.
    :return: download success or not
    """	
    output_dir = args.odir
    subset_opt = args.subset
    level4_opt = args.only_level4
    hash_name  = args.hash
    is_clean_cache = args.clean_cache

    # import pdb; pdb.set_trace()
    os.makedirs(output_dir, exist_ok=True)

    # STEP 1: download the benchmark-meta.csv and .cache/filelist.bin
    meta_repo_path = join(repo_root, 'benchmark-meta.csv')
    cache_file_path = join(repo_root, '.cache/filelist.bin')
    if hf_download_path(meta_repo_path, output_dir) == False:
        print('ERROR: Download benchmark-meta.csv failed.')
        return False

    if hf_download_path(cache_file_path, output_dir) == False:
        print('ERROR: Download .cache/filelist.bin failed.')
        return False


    # STEP 2: download the specific subset
    df = pd.read_csv(join(output_dir, 'benchmark-meta.csv'))
    filepath_dict = pickle.load(open(join(output_dir, '.cache/filelist.bin'), 'rb'))
    hashlist = df['hash'].tolist()
    download_list = hashlist

    # sanity check 
    if subset_opt == 'hash':  
        if hash_name not in hashlist: 
            print(f'ERROR: hash {hash_name} not in the benchmark-meta.csv')
            return False

        # if subset is hash, only download the specific hash
        download_list = [hash_name]

    
    # download the dataset 
    for cur_hash in tqdm(download_list):
        if download_by_hash(filepath_dict, output_dir, cur_hash, level4_opt) == False:
            return False

        if is_clean_cache:
            clean_huggingface_cache(join(output_dir, '.cache'))

    return True 


def process_one_scene(scene):
    """
    scene: path to one scene folder, no trailing /
    """
    print(scene)
    scene_name = scene.split('/')[-1].split('\\')[-1]
    json_file = scene + '/nerfstudio/transforms.json'
    json_data = json.load(open(json_file, 'r'))
    new_json_file = scene + '/opencv_cameras.json'
    new_data = {"scene_name": scene_name, "frames": []}
    w_ = json_data['w']
    h_ = json_data['h']
    fx_ = json_data['fl_x']
    fy_ = json_data['fl_y']
    cx_ = json_data['cx']
    cy_ = json_data['cy']
    k1,k2,p1,p2 = json_data['k1'], json_data['k2'], json_data['p1'], json_data['p2']
    distort = np.asarray([k1,k2,p1,p2])
    if h_ > w_:
        print("skip vertical videos for now", scene, h_,w_)
        return
    num_frams = len(json_data['frames'])
    print("num_frames: ", num_frams)

    # create undistort folder
    os.makedirs(scene+'/images_undistort', exist_ok=True)

    for i in range(num_frams):
        frame = json_data['frames'][i]
        file_path = 'images_4/' + frame['file_path'].split('/')[-1]

        # undistort
        image = cv2.imread(scene+'/nerfstudio/'+file_path, cv2.IMREAD_COLOR)
        h,w,_ = image.shape
        fx,fy,cx,cy = fx_/w_*w ,fy_/h_*h,cx_/w_*w,cy_/h_*h
        intr = np.asarray([[fx,0,cx],[0,fy,cy],[0,0,1]])

        new_intr, roi = cv2.getOptimalNewCameraMatrix(intr, distort, (w,h), 0, (w,h))
        dst = cv2.undistort(image, intr, distort, None, new_intr)
        image = dst
        h,w,_ = image.shape
        fx,fy,cx,cy = new_intr[0,0], new_intr[1,1], new_intr[0,2], new_intr[1,2]

        file_path = 'images_undistort/' + frame['file_path'].split('/')[-1]
        cv2.imwrite(scene+'/'+file_path, image)

        c2w = np.asarray(frame["transform_matrix"])
        c2w[0:3, 1:3] *= -1
        c2w = c2w[[1,0,2,3], :]
        c2w[2,:] *= -1
        w2c = np.linalg.inv(c2w)
        frame_new = {"file_path": file_path, "w2c": w2c.tolist(), "h": h, "w": w, "fx": fx, "fy": fy, "cx": cx, "cy": cy}
        new_data["frames"].append(frame_new)
    json.dump(new_data, open(new_json_file, 'w'), indent=4)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--odir', type=str, help='output directory', default='data/dl3dv_undistort_hr_test')
    parser.add_argument('--subset', choices=['full', 'hash'], help='The subset of the benchmark to download', required=True)
    parser.add_argument('--only_level4', action='store_true', help='If set, only the images_4 resolution level will be downloaded to save space')
    parser.add_argument('--clean_cache', action='store_true', help='If set, will clean the huggingface cache to save space')
    parser.add_argument('--hash', type=str, help='If set subset=hash, this is the hash code of the scene to download', default='')
    params = parser.parse_args()


    if download_benchmark(params):
        print('Download Done. Refer to', params.odir)
        process_one_scene(os.path.join(params.odir, params.hash))
    else:
        print(f'Download to {params.odir} Failed. See error messsage.')
