import os
import json
import shutil
import re


class ConfigCreator:
    def __init__(self, args):
        self.atomer_files = None
        self.args = args

    def run(self):
        self.get_atomer_output_files()
        self.create_confs()
        print("New config files created in " + self.args.result_config_dir)

    def create_confs(self):
        """
        Processes each Atomer JSON file to generate a corresponding configuration.
        For each file:
          - Creates a new result config directory named <filename>_conf.
          - Extracts functions to analyze directly from the atomer output.
          - Update the functions/include filter with those functions.
        """
        for filepath in self.atomer_files:
            filename = os.path.splitext(os.path.basename(filepath))[0]
            conf_name = f"{filename}_conf"

            # Step 1: Create base config and get path to the new config
            result_config_path = self.create_base_result_config(conf_name)

            # Step 2: Extract json from file
            json_content = self.read_json_from_file(filepath)

            # Step 3: Extract functions and files to analyze
            functions = self.extract_functions_to_analyze(json_content)

            # Step 4: Update the filter include file with the extracted functions
            self.update_functions_include(functions, result_config_path)

    @staticmethod
    def update_functions_include(functions_to_include, path_to_config):
        """
        Appends functions to the include filter file located at
        <path_to_config>/filters/functions/include. Creates the file and parent
        directories if they don't exist.

        :param functions_to_include: A set of function names to include.
        :param path_to_config: The root path to the configuration directory.
        """
        include_path = os.path.join(path_to_config, "filters", "functions", "include")

        # Ensure the directory exists
        os.makedirs(os.path.dirname(include_path), exist_ok=True)

        # Open the file in append mode, create it if it doesn't exist
        with open(include_path, "a") as f:
            for func in sorted(functions_to_include):  # Sort for consistency
                f.write(func + "\n")

    def create_base_result_config(self, conf_name):
        """
        Copies the base configuration directory and its contents to a new directory
        named `conf_name` inside the result configuration directory.

        :param conf_name: The name of the new configuration directory
        """
        src = self.args.base_config_dir
        dst = os.path.join(self.args.result_config_dir, conf_name)

        # Ensure the destination directory does not already exist
        if os.path.exists(dst):
            shutil.rmtree(dst)  # Remove it before copying to avoid conflicts

        shutil.copytree(src, dst)
        return dst

    @staticmethod
    def extract_functions_to_analyze(json_content):
        """
        Extracts a set of unique function names from the JSON content, specifically
        from the 'qualifier' field.

        :param json_content: List of JSON entries representing bugs
        :return: A set of function names
        """
        functions = set()

        for entry in json_content:
            # Extract the function name from the qualifier field
            qualifier = entry.get("qualifier", "")
            # Use regex to match the pattern that identifies the function name in qualifier
            match = re.search(r"originated in function: '([^']+)'", qualifier)
            if match:
                function_name = match.group(1)
                functions.add(function_name)

        return functions

    @staticmethod
    def read_json_from_file(filename):
        """Reads JSON content from the given file and returns it as a Python object."""
        try:
            with open(filename, 'r', encoding='utf-8') as f:
                data = json.load(f)
            return data
        except FileNotFoundError:
            print(f"Error: File not found - {filename}")
        except json.JSONDecodeError as e:
            print(f"Error decoding JSON in file {filename}: {e}")
        except Exception as e:
            print(f"Unexpected error reading file {filename}: {e}")
        return None

    def get_atomer_output_files(self):
        """Collect all .json files from the atomer_outputs_dir and store them in self.atomer_files."""
        atomer_dir = self.args.atomer_outputs_dir
        if not os.path.isdir(atomer_dir):
            raise ValueError(f"Provided atomer_outputs_dir does not exist or is not a directory: {atomer_dir}")

        self.atomer_files = [
            os.path.join(atomer_dir, f)
            for f in os.listdir(atomer_dir)
            if f.endswith('.json')
        ]
