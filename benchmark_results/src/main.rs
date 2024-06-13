//! Again, I am using Rust as a scripting language, because I really like it.
//! This is not supposed to be good Rust code.

use std::{cmp::Ordering, collections::HashMap};

#[derive(serde::Deserialize, Debug, Clone)]
enum TimeUnit {
    #[serde(rename = "ns")]
    Nanoseconds,
}

#[derive(serde::Deserialize, Debug, Clone)]
struct BenchmarkContext {}

#[allow(unused)]
#[derive(serde::Deserialize, Debug, Clone)]
struct BenchmarkResult {
    name: String,
    iterations: u64,
    real_time: f64,
    cpu_time: f64,
    time_unit: TimeUnit,
}
impl BenchmarkResult {
    fn get_tn(&self) -> (&str, &str) {
        let split = self.name.split("::").collect::<Vec<_>>();
        let t = {
            let fqt = split[0];
            let fqt = fqt.split('<').collect::<Vec<_>>()[1];
            let fqt = fqt.split('>').collect::<Vec<_>>()[0];
            fqt
        };
        let n = split[1].trim_start_matches("BM_");
        (t, n)
    }
}

#[derive(serde::Deserialize, Debug, Clone)]
struct BenchmarkResults {
    #[allow(unused)]
    context: BenchmarkContext,
    benchmarks: Vec<BenchmarkResult>,
}

impl BenchmarkResults {
    fn compare_with(&self, s: &str) -> Vec<(String, Vec<(String, f64)>)> {
        let mut umap_bms: HashMap<String, f64> = HashMap::new();
        for bm in self.benchmarks.iter().filter(|bm| bm.name.starts_with(s)) {
            let (_, n) = bm.get_tn();
            umap_bms.insert(n.to_owned(), bm.cpu_time);
        }
        let mut other_bms: HashMap<String, HashMap<String, f64>> = HashMap::new();
        for bm in self.benchmarks.iter() {
            let (t, n) = bm.get_tn();
            if !other_bms.contains_key(t) {
                other_bms.insert(t.to_owned(), HashMap::new());
            }
            other_bms
                .get_mut(t)
                .unwrap()
                .insert(n.to_owned(), umap_bms.get(n).unwrap() / bm.cpu_time);
        }
        other_bms
            .into_iter()
            .map(|(k, v)| {
                (k, {
                    let mut v = v.into_iter().collect::<Vec<_>>();
                    v.sort_by(|(_, time), (_, time2)| {
                        time.partial_cmp(time2).unwrap_or(Ordering::Equal)
                    });
                    v
                })
            })
            .collect()
    }
}

fn main() {
    let results: BenchmarkResults = serde_json::from_str(include_str!("../c.json")).unwrap();
    let relative_results = results.compare_with("MapBenchmarks<default_std_unordered_map_t>");
    for (name, bms) in relative_results {
        for (bm_name, t) in bms {
            println!("{name}|{bm_name}|{t}");
        }
    }
}
