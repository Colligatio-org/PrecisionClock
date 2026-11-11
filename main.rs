use std::io::{self, Write};
use chrono::{NaiveTime, Utc, TimeZone, Local, FixedOffset, NaiveDateTime, DateTime, Offset};
use chrono_tz::{Tz, America, Europe, Asia, Australia, Pacific, Africa};
use arboard::Clipboard;

enum MyTz {
    Named(Tz),
    Fixed(FixedOffset, String),
    Local,
}

fn get_tz_full_name(tz_str: &str) -> String {
    let upper = tz_str.to_uppercase();
    match upper.as_str() {
        "EST" | "EDT" | "AMERICA/NEW_YORK" | "NYC" | "NEWYORK" => "Eastern Time".to_string(),
        "CST" | "CDT" | "AMERICA/CHICAGO" | "CHICAGO" => "Central Time".to_string(),
        "MST" | "MDT" | "AMERICA/DENVER" | "DENVER" => "Mountain Time".to_string(),
        "PST" | "PDT" | "AMERICA/LOS_ANGELES" | "LA" | "SF" | "LOSANGELES" => "Pacific Time".to_string(),
        "GMT" | "BST" | "EUROPE/LONDON" | "LONDON" | "UK" => "Greenwich Mean Time".to_string(),
        "CET" | "CEST" | "EUROPE/BERLIN" | "BERLIN" | "PARIS" => "Central European Time".to_string(),
        "EET" | "EEST" | "EUROPE/ATHENS" => "Eastern European Time".to_string(),
        "MSK" | "EUROPE/MOSCOW" | "MOSCOW" => "Moscow Standard Time".to_string(),
        "JST" | "ASIA/TOKYO" | "TOKYO" | "JP" => "Japan Standard Time".to_string(),
        "KST" | "ASIA/SEOUL" | "SEOUL" => "Korea Standard Time".to_string(),
        "CN" | "CST_CN" | "ASIA/SHANGHAI" | "SHANGHAI" | "BEIJING" | "BJ" => "China Standard Time".to_string(),
        "HKT" | "ASIA/HONG_KONG" | "HK" => "Hong Kong Time".to_string(),
        "SGT" | "ASIA/SINGAPORE" | "SG" => "Singapore Time".to_string(),
        "IST" | "ASIA/KOLKATA" | "INDIA" => "India Standard Time".to_string(),
        "AEST" | "AEDT" | "AUSTRALIA/SYDNEY" | "SYDNEY" => "Australian Eastern Time".to_string(),
        "NZST" | "NZDT" | "PACIFIC/AUCKLAND" | "AUCKLAND" => "New Zealand Standard Time".to_string(),
        "AFRICA/CAIRO" | "CAIRO" => "Eastern European Time".to_string(),
        "AFRICA/JOHANNESBURG" | "JOHANNESBURG" => "South Africa Standard Time".to_string(),
        "UTC" | "Z" => "Coordinated Universal Time".to_string(),
        "LOCAL" => "Local Time".to_string(),
        _ => format!("{} Time", tz_str),
    }
}

fn parse_tz(tz_str: &str) -> Option<MyTz> {
    let upper = tz_str.to_uppercase();
    if upper == "LOCAL" { return Some(MyTz::Local); }
    
    if upper.starts_with("GMT") || upper.starts_with("UTC") {
        if upper.len() >= 5 {
            let sign = &upper[3..4];
            let hours_str = &upper[4..];
            if let Ok(hours) = hours_str.parse::<i32>() {
                let offset_sec = if sign == "+" { hours * 3600 } else { -hours * 3600 };
                if let Some(offset) = FixedOffset::east_opt(offset_sec) {
                    return Some(MyTz::Fixed(offset, format!("GMT{}{}", sign, hours)));
                }
            }
        }
    }
    
    let tz = match upper.as_str() {
        "EST" | "EDT" | "NYC" | "NEWYORK" => America::New_York,
        "CST" | "CDT" | "CHICAGO" => America::Chicago,
        "MST" | "MDT" | "DENVER" => America::Denver,
        "PST" | "PDT" | "LA" | "SF" | "LOSANGELES" => America::Los_Angeles,
        "AKST" | "AKDT" => America::Anchorage,
        "HST" => Pacific::Honolulu,
        "GMT" | "BST" | "LONDON" | "UK" => Europe::London,
        "WET" | "WEST" => Europe::Lisbon,
        "CET" | "CEST" | "BERLIN" | "PARIS" => Europe::Berlin,
        "EET" | "EEST" | "ATHENS" => Europe::Athens,
        "MSK" | "MOSCOW" => Europe::Moscow,
        "JST" | "TOKYO" | "JP" => Asia::Tokyo,
        "KST" | "SEOUL" => Asia::Seoul,
        "CN" | "CST_CN" | "SHANGHAI" | "BEIJING" | "BJ" => Asia::Shanghai,
        "HKT" | "HK" => Asia::Hong_Kong,
        "SGT" | "SG" => Asia::Singapore,
        "IST" | "INDIA" => Asia::Kolkata,
        "AEST" | "AEDT" | "SYDNEY" => Australia::Sydney,
        "NZST" | "NZDT" | "AUCKLAND" => Pacific::Auckland,
        "CAIRO" => Africa::Cairo,
        "JOHANNESBURG" => Africa::Johannesburg,
        "UTC" | "Z" => chrono_tz::UTC,
        _ => tz_str.parse::<Tz>().ok()?,
    };
    Some(MyTz::Named(tz))
}

fn to_utc(tz: &MyTz, naive: NaiveDateTime) -> Option<DateTime<Utc>> {
    match tz {
        MyTz::Named(t) => match t.from_local_datetime(&naive) {
            chrono::LocalResult::Single(dt) => Some(dt.with_timezone(&Utc)),
            chrono::LocalResult::Ambiguous(dt, _) => Some(dt.with_timezone(&Utc)),
            chrono::LocalResult::None => None,
        },
        MyTz::Fixed(offset, _) => match offset.from_local_datetime(&naive) {
            chrono::LocalResult::Single(dt) => Some(dt.with_timezone(&Utc)),
            _ => None,
        },
        MyTz::Local => match Local.from_local_datetime(&naive) {
            chrono::LocalResult::Single(dt) => Some(dt.with_timezone(&Utc)),
            chrono::LocalResult::Ambiguous(dt, _) => Some(dt.with_timezone(&Utc)),
            chrono::LocalResult::None => None,
        },
    }
}

fn parse_time(s: &str) -> Option<NaiveTime> {
    if s.eq_ignore_ascii_case("now") { return None; }
    let s_lower = s.to_lowercase();
    let (is_pm, is_am);
    let mut time_str = s_lower.as_str();
    
    if time_str.ends_with("pm") { is_pm = true; is_am = false; time_str = &time_str[..time_str.len()-2]; }
    else if time_str.ends_with("am") { is_pm = false; is_am = true; time_str = &time_str[..time_str.len()-2]; }
    else { is_pm = false; is_am = false; }
    
    let parts: Vec<&str> = time_str.trim().split(':').collect();
    if parts.is_empty() || parts.len() > 2 { return None; }
    
    let h_str = parts[0];
    let m_str = if parts.len() == 2 { parts[1] } else { "0" };
    
    let mut h: u32 = h_str.parse().ok()?;
    let m: u32 = m_str.parse().ok()?;
    if m >= 60 { return None; }
    
    if is_pm || is_am {
        if h == 0 || h > 12 { return None; }
        if is_pm && h != 12 { h += 12; }
        if is_am && h == 12 { h = 0; }
    } else { if h > 23 { return None; } }
    
    NaiveTime::from_hms_opt(h, m, 0)
}

fn format_target(utc_dt: DateTime<Utc>, tz: &MyTz, tz_str: &str) -> (String, String, String, i32) {
    match tz {
        MyTz::Named(t) => {
            let dt = utc_dt.with_timezone(t);
            let offset = dt.offset().fix().local_minus_utc() / 3600;
            let time_str = dt.format("%I:%M %p").to_string().trim_start_matches('0').to_string();
            let abbr = dt.format("%Z").to_string();
            (time_str, abbr, get_tz_full_name(tz_str), offset)
        }
        MyTz::Fixed(offset, name) => {
            let dt = utc_dt.with_timezone(offset);
            let offset_hours = offset.local_minus_utc() / 3600;
            let time_str = dt.format("%I:%M %p").to_string().trim_start_matches('0').to_string();
            (time_str, name.clone(), format!("{} Time", name), offset_hours)
        }
        MyTz::Local => {
            let local_tz = Local::now().timezone();
            let dt = utc_dt.with_timezone(&local_tz);
            let offset = dt.offset().fix().local_minus_utc() / 3600;
            let time_str = dt.format("%I:%M %p").to_string().trim_start_matches('0').to_string();
            let abbr = dt.format("%Z").to_string();
            (time_str, abbr, "Local Time".to_string(), offset)
        }
    }
}

fn to_tz_offset(tz: &MyTz) -> FixedOffset {
    match tz {
        MyTz::Named(t) => Utc::now().with_timezone(t).offset().fix(),
        MyTz::Fixed(offset, _) => *offset,
        MyTz::Local => Utc::now().with_timezone(&Local).offset().fix(),
    }
}

fn process_command(input: &str) {
    let parts: Vec<&str> = input.split_whitespace().collect();
    if parts.is_empty() { return; }

    let time_str;
    let from_str;
    let to_str;

    if parts[0].eq_ignore_ascii_case("time") && parts.len() == 2 {
        time_str = "now"; from_str = parts[1]; to_str = parts[1];
    } else if parts.len() == 4 && (parts[2].eq_ignore_ascii_case("to") || parts[2].eq_ignore_ascii_case("in")) {
        time_str = parts[0]; from_str = parts[1]; to_str = parts[3];
    } else if parts.len() == 5 && parts[1].eq_ignore_ascii_case("from") && parts[3].eq_ignore_ascii_case("to") {
        time_str = parts[0]; from_str = parts[2]; to_str = parts[4];
    } else if parts.len() == 3 && parts[1].eq_ignore_ascii_case("in") {
        time_str = parts[0]; from_str = "Local"; to_str = parts[2];
    } else if parts.len() == 2 && parts[0].eq_ignore_ascii_case("now") {
        time_str = "now"; from_str = "Local"; to_str = parts[1];
    } else {
        println!("\n[!] Invalid format. Try: 3pm EST to PST | 3pm in PST | now tokyo in london | 9am GMT+8 in UTC | time EST\n");
        return;
    }

    let from_tz = match parse_tz(from_str) { Some(t) => t, None => { println!("\n[!] Unknown source timezone: {}\n", from_str); return; } };
    let to_tz = match parse_tz(to_str) { Some(t) => t, None => { println!("\n[!] Unknown target timezone: {}\n", to_str); return; } };

    let utc_dt = if time_str.eq_ignore_ascii_case("now") {
        Utc::now()
    } else {
        let time = match parse_time(time_str) { Some(t) => t, None => { println!("\n[!] Invalid time format: {}\n", time_str); return; } };
        let naive_dt = Local::now().date_naive().and_time(time);
        match to_utc(&from_tz, naive_dt) {
            Some(dt) => dt,
            None => { println!("\n[!] The time is invalid in {} due to DST gap.\n", from_str); return; }
        }
    };

    let (target_time, target_abbr, target_name, target_offset) = format_target(utc_dt, &to_tz, to_str);
    
    let local_now = Local::now();
    let target_naive = utc_dt.with_timezone(&to_tz_offset(&to_tz));
    let day_diff = target_naive.date_naive().signed_duration_since(local_now.date_naive()).num_days();
    let day_str = match day_diff { 0 => "Today".to_string(), 1 => "+1 day".to_string(), -1 => "-1 day".to_string(), d if d > 0 => format!("+{} days", d), d => format!("{} days", d) };

    let hour_diff = target_offset - (local_now.offset().fix().local_minus_utc() / 3600);
    let diff_str = if hour_diff == 0 { "Same as local".to_string() } else if hour_diff > 0 { format!("{} hours ahead of local", hour_diff) } else { format!("{} hours behind local", hour_diff.abs()) };

    let main_title = format!("{} {} . {}", target_time, target_abbr, day_str);
    let sub_title = format!("{} (UTC{:+}) . {}", target_name, target_offset, diff_str);

    println!("\n----------------------------------------");
    println!(" {}", main_title); 
    println!(" {}", sub_title);       
    println!("----------------------------------------");

    if let Ok(mut ctx) = Clipboard::new() {
        if ctx.set_text(format!("{}\n{}", main_title, sub_title)).is_ok() {
            println!(" [OK] Copied to clipboard\n");
        }
    }
}

fn main() {
    println!("=== Precision Clock Timezone Converter (CLI) v0.4.1 ===");
    println!("No network. Pure local offline computation.");
    println!("Supports full IANA timezones and DST.\n");
    println!("Example: 3pm EST to PST | 3pm in PST | now tokyo in london | time EST");
    println!("Type 'exit' or 'quit' to quit.\n");

    let stdin = io::stdin();
    loop {
        print!("> ");
        io::stdout().flush().unwrap();
        let mut input = String::new();
        if stdin.read_line(&mut input).is_err() { break; }
        let input = input.trim();
        if input.is_empty() { continue; }
        if input.eq_ignore_ascii_case("exit") || input.eq_ignore_ascii_case("quit") { println!("Goodbye."); break; }
        process_command(input);
    }
}
