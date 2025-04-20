from src.ArgumentParser import MyArgumentParser
from src.ConfigCreator import ConfigCreator

def main():
    # Deal with input arguments
    parser = MyArgumentParser()
    args = parser.parse_args()
    parser.ensure_result_config_dir_exists(args)

    # Create the desired configuration dir
    ConfigCreator(args).run()

if __name__ == "__main__":
    main()
