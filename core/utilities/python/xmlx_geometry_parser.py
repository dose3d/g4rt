#!/usr/bin/env python3
import pandas as pd
import ast
import argparse

def parse_row(row):
    # Parsuj listy za pomocą ast.literal_eval
    nodes   = ast.literal_eval(row["Mesh_Nodes"])
    verts   = ast.literal_eval(row["Mesh_Vertices"])
    normals = ast.literal_eval(row["Mesh_Normal_Vectors"])
    com = [
        float(row["Co_M_X"]),
        float(row["Co_M_Y"]),
        float(row["Co_M_Z"])
    ]
    return {
        "component": row["Component_Name"],
        "body":      row["Body_Name"],
        "com":       com,
        "sc_id":     row["SC_id"] or "",
        "nodes":     nodes,
        "vertices":  verts,
        "normals":   normals
    }

def get_geometries(filename: str, sheet_name=None):
    df = pd.read_excel(
        filename,
        sheet_name=(0 if sheet_name is None else sheet_name),
        engine="openpyxl",
        dtype=str
    )
    required = [
      "Component_Name","Body_Name",
      "Co_M_X","Co_M_Y","Co_M_Z",
      "SC_id","Mesh_Nodes",
      "Mesh_Vertices","Mesh_Normal_Vectors"
    ]
    for c in required:
        if c not in df.columns:
            raise KeyError(f"Brakuje kolumny: {c}")
    geoms = []
    for idx,row in df.iterrows():
        # tylko te wiersze, które mają cokolwiek w Mesh_Vertices
        if pd.isna(row["Mesh_Vertices"]): 
            continue
        geoms.append(parse_row(row))
    return geoms

if __name__=="__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("filename")
    parser.add_argument("--sheet", default=None)
    args = parser.parse_args()
    geoms = get_geometries(args.filename, args.sheet)
    print(f"Parsed {len(geoms)} geometries")
