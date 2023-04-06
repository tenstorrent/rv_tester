#!/opt/miniconda3/bin/python3
import argparse
import getpass
import glob
import os.path
import re
import pandas as pd
import sqlalchemy
import yaml
import io
import json
from datetime import datetime


class DatabaseWriter:

    @staticmethod
    def to_version_str(version):
        min_len = 8
        if version is None or len(version) < min_len:
            raise ValueError("Version string is missing or too short '%s', min length is %d" % (str(version), min_len))
        err = re.findall(r'([^0-9a-fA-F]+)', version)
        if err:
            raise ValueError("Unexpected Version '%s', illegal character %s" % (str(version), str(err)))
        return version.lower()[0:min_len]

    def __init__(self, host: str, port: int, database_name: str, version: str, database_prefix='r', echo=False):
        self.version = self.to_version_str(version)
        self.full_database_name = "%s%s_%s" % (str(database_prefix), self.version, str(database_name))

        self.engine = sqlalchemy.create_engine("mysql+pymysql://%s:%s@%s:%d" % ("writer", "writer", host, port),
                                               echo=echo)
        self.engine.execute("CREATE DATABASE IF NOT EXISTS `%s`;" % self.full_database_name)
        self.engine.execute("USE `%s`" % self.full_database_name)

    def get_version(self):
        return self.version

    def get_database_name(self):
        return self.full_database_name

    def get_engine(self):
        return self.engine

    def get_url(self):
        return self.engine.url

    def get_table(self, table_name):
        metadata = sqlalchemy.MetaData()
        return sqlalchemy.Table(table_name, metadata, autoload_with=self.engine)

    def del_table(self, table_name):
        self.engine.execute("DROP TABLE IF EXISTS `%s`" % table_name)

    def clear_database(self):
        for table_name in self.engine.execute("show tables").fetchall():
            for t in table_name:
                self.del_table(t)

    def add_id_column(self, table_name):
        self.engine.execute(
            "ALTER TABLE `%s` ADD COLUMN `_id_` INT UNSIGNED PRIMARY KEY AUTO_INCREMENT FIRST" % table_name)

    def show_tables(self):
        print(self.engine.execute("show tables").fetchall())

    @staticmethod
    def upload_all_dataframes(host: str, port: int, database_name: str, version: str, dataframes_source, reset: bool,
                              echo: bool, argparse_options):
        db_writer = DatabaseWriter(host=host, port=port, database_name=database_name, version=argparse_options.version,
                                   echo=echo)
        if reset:
            db_writer.clear_database()
        now = datetime.utcnow()
        file_path = argparse_options.file_path
        if file_path is not None:
            file_path = os.path.abspath(file_path)
        pd.DataFrame({  # Please make sure this table is uploaded first for traceability
            'description': ['AS Interface Spec'],
            'utc_time_string': [str(now)],
            'utc_time': [now],
            'user': [getpass.getuser()],
            'version': [version],
            'reset': [reset],
            'dataframes_source': [type(dataframes_source)],
            'script': [__file__],
            'argparse': [str(argparse_options)],
            'file_path': [str(file_path)],
            'tag': [argparse_options.tag],
            'uname': [os.uname()]
        }).to_sql(name="__database__secrets__", con=db_writer.get_engine(), if_exists='append', index=False)

        type_override = {
            'fixme_MAX': sqlalchemy.INT,
            'fixme_MIN': sqlalchemy.INT,
            'NAME': sqlalchemy.TEXT
        }
        df_names = list()
        for name, value in vars(dataframes_source).items():
            if isinstance(value, pd.DataFrame):
                table = str(name)
                df_names.append(table)
                if True or argparse_options.verbose:
                    print("%s: uploading version '%s' to '%s %s.%s'" % (__file__, db_writer.get_version(),
                                                                        str(db_writer.get_url()),
                                                                        db_writer.get_database_name(), table))
                value.to_sql(name=table, con=db_writer.get_engine(),
                             if_exists='replace', index=False, dtype=type_override)
                db_writer.add_id_column(table)

    @staticmethod
    def save_xlsx_interface(interface, path):  # note this require: pip install openpyxl
        path += '.xlsx'
        if os.path.exists(path):
            raise RuntimeError('xlsx file "%s" already exist.' % path)
        writer = pd.ExcelWriter(path=path)
        for name, value in vars(interface).items():
            if isinstance(value, pd.DataFrame):
                value.to_excel(excel_writer=writer, sheet_name=name)
        writer.save()
        return path

    @staticmethod
    def upload_xlsx_interface(path):
        path += '.xlsx'

        class DataframeContainer:
            def __init__(self, dataframe_source):
                for sheet, df in dataframe_source.items():
                    setattr(self, sheet, df)

        return path, DataframeContainer(pd.read_excel(io=path, sheet_name=None, engine='openpyxl'))

    @staticmethod
    def save_csvs_interface(interface, path):
        if os.path.exists(path):
            raise RuntimeError('csv file "%s" already exist.' % path)
        os.mkdir(path)
        for name, value in vars(interface).items():
            if isinstance(value, pd.DataFrame):
                # noinspection PyTypeChecker
                value.to_csv(os.path.join(path, "%s.csv" % name))
        return path

    @staticmethod
    def upload_csvs_interface(path):
        class DataframeContainer:
            def __init__(self, dataframe_source):
                for name, df in dataframe_source.items():
                    setattr(self, name, df)

        csv_dfs = dict()
        for csv in glob.glob(os.path.join(path, "*.csv")):
            csv_dfs[os.path.splitext(os.path.basename(csv))[0]] = pd.read_csv(csv)
        if len(csv_dfs) < 1:
            raise FileNotFoundError('failed to find "*.csv" in "%s"' % path)
        return path, DataframeContainer(csv_dfs)

    @staticmethod
    def save_csv_interface(interface, path):
        path += '.csv'
        if os.path.exists(path):
            raise RuntimeError('csv file "%s" already exist.' % path)
        df = pd.DataFrame()
        for name, value in vars(interface).items():
            if isinstance(value, pd.DataFrame):
                df.at[0, name] = value.to_csv(index=False)
        df.to_csv(path, index=False)
        return path

    @staticmethod
    def upload_csv_interface(path):
        path += '.csv'

        class DataframeContainer:
            def __init__(self, dataframe_source):
                for name, df in dataframe_source.items():
                    setattr(self, name, pd.read_csv(io.StringIO(df[0]), sep=','))

        with open(path, 'r') as csv_in:
            return path, DataframeContainer(pd.read_csv(csv_in))

    @staticmethod
    def save_xml_interface(interface, path):
        path += '.xml'
        if os.path.exists(path):
            raise RuntimeError('xml file "%s" already exist.' % path)
        df = pd.DataFrame()
        for name, value in vars(interface).items():
            if isinstance(value, pd.DataFrame):
                df.at[0, name] = value.to_xml(index=False)
        df.to_xml(path, index=False)
        return path

    @staticmethod
    def upload_xml_interface(path):
        path += '.xml'

        class DataframeContainer:
            def __init__(self, dataframe_source):
                for name, df in dataframe_source.items():
                    setattr(self, name, pd.read_xml(df[0]))

        with open(path, 'r') as xml_in:
            return path, DataframeContainer(pd.read_xml(xml_in))

    @staticmethod
    def save_json_interface(interface, path):
        path += '.json'
        if os.path.exists(path):
            raise RuntimeError('json file "%s" already exist.' % path)
        data_dict = dict()
        for name, value in vars(interface).items():
            if isinstance(value, pd.DataFrame):
                data_dict[name] = value.to_dict()
        with open(path, 'w') as json_out:
            json_out.write(json.dumps(obj=data_dict, indent=4))
            json_out.close()
        return path

    @staticmethod
    def upload_json_interface(path):
        path += '.json'

        class DataframeContainer:
            def __init__(self, dataframe_source):
                for name, data_dict in dataframe_source.items():
                    setattr(self, name, pd.DataFrame(data_dict))

        with open(path, 'r') as json_in:
            return path, DataframeContainer(json.load(json_in))

    @staticmethod
    def save_yaml_interface(interface, path):
        path += '.yaml'
        if os.path.exists(path):
            raise RuntimeError('yaml file "%s" already exist.' % path)
        data_dict = dict()
        for name, value in vars(interface).items():
            if isinstance(value, pd.DataFrame):
                data_dict[name] = value.to_dict()
        with open(path, 'w') as yaml_out:
            yaml.dump(data_dict, yaml_out, width=float("inf"))
        return path

    @staticmethod
    def upload_yaml_interface(path):
        path += '.yaml'

        class DataframeContainer:
            def __init__(self, dataframe_source):
                for name, data_dict in dataframe_source.items():
                    setattr(self, name, pd.DataFrame(data_dict))

        with open(path, 'r') as yaml_in:
            return path, DataframeContainer(yaml.load(yaml_in, yaml.BaseLoader))

    @staticmethod
    def save_html_interface(interface, path):
        path += '.html'
        if os.path.exists(path):
            raise RuntimeError('html file "%s" already exist.' % path)
        data_dict = dict()
        for name, value in vars(interface).items():
            if isinstance(value, pd.DataFrame):
                data_dict[name] = value.to_html()
        with open(path, 'w') as yaml_out:
            # need to use yaml to avoid nested html tables, another way is to escape nested html tables
            yaml.dump(data_dict, yaml_out, width=float("inf"))
        return path

    @staticmethod
    def upload_html_interface(path):
        path += '.html'

        class DataframeContainer:
            def __init__(self, dataframe_source):
                for name, df in dataframe_source.items():
                    setattr(self, name, pd.read_html(df)[0])

        with open(path, 'r') as yaml_in:
            return path, DataframeContainer(yaml.load(yaml_in, yaml.BaseLoader))


if __name__ == "__main__":
    formats = ['xlsx', 'csvs']
    file_action_description = pd.DataFrame(
        columns=['-file_action', 'DESCRIPTION'], data=[
            ['save_xlsx', 'store each "Dataframe" to a Excel sheet and save it as a ".xlsx" file.'],
            ['upload_xlsx', 'upload from a single ".xlsx" file to a remote "MySql" database directly.'],
            ['save_csvs', '''create a directory and save each "Dataframe" to it's own ".csv" file.'''],
            ['upload_csvs', 'upload from a directory of ".csv" files to a remote "MySql" database directly.']
        ])
    for f in ['json', 'yaml', 'csv', 'html', 'xml']:
        formats.append(f)
        file_action_description.loc[len(file_action_description.index)] = [
            'save_%s' % f, 'store all "Dataframes" to a single ".%s" file.' % f]
        file_action_description.loc[len(file_action_description.index)] = [
            'upload_%s' % f, 'upload from a single ".%s" file to a remote "MySql" database directly.' % f]

    parser = argparse.ArgumentParser('''Description: Script to create Ascalon interface database from DataFrame.
Please see document at https://pandas.pydata.org/docs/reference/api/pandas.DataFrame.html

Yu Sun's preference is to code spec data directly as Python modules using pandas.DataFrame objects that can be imported 
directly by other Python scripts which is the most efficient, straightforward and less error prone way of standardizing 
this process.
    1.  Consider Dataframe as a fancy dynamic 2D array for describing and storing data in tabular format.
    2.  It's way more powerful than just a plain static data storing format such as CSV.
        Instead, DataFrame is a prevalent open source Python module based on the most popular programming language in 
        the world according to https://www.futurelearn.com/info/insights/python-insights
    3.  If all you want to do is describe data in a static declarative format, then please look at how the  
        "file_action_description" DataFrame is initialized in this script.
        (%s)
        There are many ways to initialize and store data in a DataFrame.  This format was chosen to demonstrate that 
        the overhead of using DataFrame is almost negligible and it's very readable.  Notice the different quotation 
        marks were used to avoid having to escape Python reserved characters.  Of course data can be further padded 
        with extra white spaces to align columns and rows in accordance with Python syntax, however, this script was 
        automatically formatted by a Python editor enforcing PEP8 formatting rules.
    4.  Agreed, adding rows and columns in Excel is relatively easy. However, this script also demonstrated one of the 
        many ways to utilize native Python programming code to dynamically add additional rows and columns as in the 
        example where other popular file formats were also added.  In addition to native Python capabilities, there are 
        countless open source Python modules to process DataFrame and billions of programmers who can read and modify 
        Python code.  No additional parser is needed because DataFrame is written in native Python language.
        Same as any other Python modules, a simple import statement can be used for sharing spec data with other Python
        scripts.  Syntax checking is done natively via Python interpreter with standard error messages.  The internet 
        is filled with Python documentations, working code solutions, training videos, discussing forums.
        Python skill is portable, additional functionality and customization code can be implemented by hiring people
        with Python experience.  When all things are considered, this minor inconvenience is hardly a trade-off 
        when comparing to parsing Excel files, enforcing syntax and formatting requirements and naming conventions,
        revision control, merging data from different Excel files, editing Excel files on Linux, sharing Excel documents
        on different computing platform, etc.
    5.  Yu Sun went against his strong belief of keeping things simple and efficient when he decided to augment this
        simple script with functionalities that most people will never use, unnecessarily complicating a data sharing
        process just to prove a point.  But he believes it's a necessary evil in this case to go out of his way to
        accommodate people with different backgrounds and preferences.  It is ok if people are still not convinced
        at least there are more choices.  The effort of coding this script further convinced him of the power of
        DataFrame and it also inspired him to apply this newly acquired knowledge to resolve another issue. 
    6.  Be forewarned, not all file format support nested data constructs, this should be expected as most simple 
        format are often not sophisticated.  There are many techniques employed in this script to overcome these 
        limitations, some may affect readability due to escape characters other may affect compatibility with other
        parser.  Also, when converting from one file format to another, data programming order are not preserved
        since most file formats are not programming language.  Ordering may or may not affect consuming applications,
        in case it does, additional meta data will be required to reconstruct back original ordering which is not done
        in this script.  These tasks will be left for the perspective user to support their own formatting preference.
        One more thing, in order to support additional formats such as Excel, this script now requires additional Python 
        modules installed on each user systems.  Depending on their OS and application version, additional variations 
        are to be expected which is outside the control of this script and the Python language itself.
        
        As of 1/30/2022: The following Python Modules are required to process .xlsx, .html and .xml formats
        /opt/miniconda3/bin/pip install typing
        /opt/miniconda3/bin/pip install lxml
        /opt/miniconda3/bin/pip install openpyxl

    ''' % __file__)
    parser.add_argument("-doc", "--doc", help="(optional) show document of supported file actions and help message",
                        action="store_true")
    parser.add_argument("-verbose", "--verbose", help="(optional) enable verbose mode", action="store_true")
    parser.add_argument("-host", "--host", type=str, help="(optional) change default database host",
                        default="sjcservo01")
    parser.add_argument("-port", "--port", type=int, help="(optional) change default database port number",
                        default=3306)
    parser.add_argument("-database_name", "--database_name", type=str, help="(optional) change default database name",
                        default="as_interface")
    parser.add_argument("-file_action", "--file_action", type=str,
                        choices=list(file_action_description['-file_action']),
                        help="(discouraged) action to perform on given file_path, supported formats are %s" % formats)
    parser.add_argument("-file_path", "--file_path", type=str, help="(required when file_action is used) file path",
                        default=os.path.join(os.path.expanduser('~'), "interface_file"))
    parser.add_argument("-reset", "--reset", help="(optional) permanently delete specified database table",
                        action="store_true")
    parser.add_argument("-version", "--version", type=str, help="(required) version is Gitlab revision control sha#")
    parser.add_argument("-tag", "--tag", type=str, help="(optional) additional user tag string, short one line message",
                        default="")
    args = parser.parse_args()
    ####################################################################################################################
    if args.doc:
        parser.print_help()
        for index, row in file_action_description.iterrows():
            print("%3s. use -file_action %-15s to %s" % (index, row['-file_action'], row['DESCRIPTION']))
        print('''
In addition to aforementioned file formats, data can be entered and modified directly via following Web GUI, which 
however is strongly discouraged primarily due to traceability concern. 

    http://sjcservo01:8887/apps/rtldb_edit

Here is an example to convert DataFrame to ".%s" format:
Please make sure to consider the effort required to update and maintain data in this format.
    %s -file_action save_%s -file_path [path to output file destination] -version [GitLab revision control sha#]
    
Here is how to upload data from above file but not the actual file to a database server located at "%s:%s"
    %s -file_action upload_%s -file_path [path to input file source location] -version [GitLab revision control sha#]
    
If you agree to use DataFrame directly, then no further action is required because this script is already integrated
with the Gitlab check-in process. However you can always perform a manual upload to the remote database server using 
the following command even directly from your laptop logged in to company network (assuming all necessary Python 
modules are already installed locally).
    %s -version [GitLab revision control sha#]

Optional argument such as -tag can be used with above command to provide additional comments stored on database.
-reset is designed to overwrite data in existing database with matching name and version number.
Please use caution as this script has remote networking capability to modify remote databases directly.
        ''' % (formats[0], __file__, formats[0], args.host, args.port,
               __file__, formats[0], __file__))
        exit(1)

    formats = "[%s]" % ','.join(formats)
    if args.file_action:
        file_action = str(args.file_action)

        if file_action.startswith('save_'):
            out_path = eval("DatabaseWriter.%s_interface(interface=as_interface, path=args.file_path)" % file_action)
            print("%s: saved to : %s" % (__file__, os.path.abspath(out_path)))
        elif file_action.startswith('upload_'):
            from_path, dfs = eval("DatabaseWriter.%s_interface(path=args.file_path)" % file_action)
            DatabaseWriter.upload_all_dataframes(host=args.host, port=args.port, database_name=args.database_name,
                                                 version=args.version, dataframes_source=dfs, reset=args.reset,
                                                 echo=args.verbose, argparse_options=args)
            print("%s: uploaded from : %s" % (__file__, os.path.abspath(from_path)))
        else:
            raise RuntimeError('Unsupported file_action "%s" is not in %s' % (file_action, formats))
    else:
        raise RuntimeError('Only file_action supported')

    exit(0)
