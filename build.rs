use std::env;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    if cfg!(target_os = "windows") {
        println!("cargo:rerun-if-changed=resource.rc");
        println!("cargo:rerun-if-changed=64x64.ico");
        
        let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
        let res_o = out_dir.join("resource.o");
        let status = Command::new("windres")
            .arg("resource.rc")
            .arg("-O").arg("coff")
            .arg("-o").arg(&res_o)
            .status().expect("windres missing");
        
        if !status.success() { panic!("windres failed"); }
        
        println!("cargo:rustc-link-arg={}", res_o.display());
    }
}